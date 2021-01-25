/*
 * Copyright (c) 2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"
#include "driver/touch_pad.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onReadable;
	uint32_t			values;
	uint8_t				closed;
	uint8_t				triggered;
} modTouchpadRecord, *modTouchpad;

#define DEFAULT_SENSITIVITY 0.2
#define MAXIMUM_PIN 31

void xs_touchpad_mark(xsMachine* the, void *it, xsMarkRoot markRoot);
static void startTouchpad(modTouchpad touchpad);
static void setThresholds(uint32_t pins, double sensitivity);
static void touchpadDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void touchpadISR(void *refcon);

static modTouchpad gTouchpad = NULL;

static const xsHostHooks ICACHE_FLASH_ATTR xsTouchpadHooks = {
	xs_touchpad_destructor,
	xs_touchpad_mark,
	NULL
};

void xs_touchpad_destructor(void *data)
{
	modTouchpad touchpad = data;
	if (NULL == touchpad)
		return;

	touch_pad_fsm_stop();
	touch_pad_intr_disable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE);
	touch_pad_isr_deregister(touchpadISR, touchpad);
	touch_pad_deinit();

    gTouchpad = NULL;
	c_free(touchpad);
}

void xs_touchpad(xsMachine *the)
{
	modTouchpad touchpad;
    int pins, tmpPins, i = 0, guardPin = -1;
	double sensitivity = DEFAULT_SENSITIVITY;

	if (gTouchpad)
        xsUnknownError("ESP32 only supports one touchpad instance");
    
    xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_pins);
	pins = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_guard)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_guard);
		guardPin = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_sensitivity)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_sensitivity);
		sensitivity = xsmcToNumber(xsVar(0));
	}	

	touchpad = c_malloc(sizeof(modTouchpadRecord));
	
	if (!touchpad)
		xsUnknownError("no memory");

    touchpad->the = the;
	touchpad->obj = xsThis;
	touchpad->onReadable = NULL;
	touchpad->values = 0;
	touchpad->closed = false;
	touchpad->triggered = false;
	
	xsRemember(touchpad->obj);
	xsmcSetHostData(xsThis, touchpad);

    touch_pad_init();

	if (guardPin != -1) {
		touch_pad_waterproof_t config = {
			.guard_ring_pad = guardPin,
			.shield_driver = TOUCH_PAD_SHIELD_DRV_L2
		};
		touch_pad_waterproof_set_config(&config);
		touch_pad_waterproof_enable();
	}

	tmpPins = pins;
	while(tmpPins) {
		if (tmpPins & 0x01)
			touch_pad_config(i);
		i++;
		tmpPins >>= 1;
	}

	if (xsmcHas(xsArg(0), xsID_onReadable)) {
		xsSlot tmp;

		xsmcGet(tmp, xsArg(0), xsID_onReadable);
		touchpad->onReadable = xsToReference(tmp);

		xsSetHostHooks(xsThis, (xsHostHooks *)&xsTouchpadHooks);
	}

	startTouchpad(touchpad);
	setThresholds(pins, sensitivity);

	gTouchpad = touchpad;
}

void xs_touchpad_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	modTouchpad touchpad = it;

	(*markRoot)(the, touchpad->onReadable);
}

void xs_touchpad_close(xsMachine *the)
{
	modTouchpad touchpad = xsmcGetHostData(xsThis);
	if (NULL == touchpad)
		return;

	xsForget(touchpad->obj);
	touchpad->closed = true;
	if (!touchpad->triggered)
		xs_touchpad_destructor(touchpad);
	xsmcSetHostData(xsThis, NULL);
}

void xs_touchpad_read(xsMachine *the)
{
    modTouchpad touchpad = xsmcGetHostData(xsThis);
	if (NULL == touchpad)
		xsUnknownError("bad state");

	xsResult = xsInteger(touchpad->values);
}

static void startTouchpad(modTouchpad touchpad)
{
	touch_filter_config_t filter_info = {
		.mode = TOUCH_PAD_FILTER_IIR_16,
		.debounce_cnt = 1,
		.noise_thr = 0,
		.jitter_step = 4,
		.smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
	};
	touch_pad_filter_set_config(&filter_info);
	touch_pad_filter_enable();

	touch_pad_isr_register(touchpadISR, touchpad, TOUCH_PAD_INTR_MASK_ALL);
	touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE);
	
	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
	touch_pad_fsm_start();

	vTaskDelay(100 / portTICK_RATE_MS); //@@ there does seem to need to be a delay between fsm_start and setting thresholds, but this may not be the best way to implement it
}

static void setThresholds(uint32_t pins, double sensitivity)
{
	int i = 0;
	uint32_t value;

	while(pins) {
		if (pins & 0x01) {
			touch_pad_read_benchmark(i, &value);
			touch_pad_set_thresh(i, value * sensitivity);
		}
		i++;
		pins >>= 1;
	}
}

void touchpadISR(void *refcon)
{
	modTouchpad touchpad = refcon;
	
	touchpad->values = touch_pad_get_status();

	if (!touchpad->onReadable)
		return;

	if (touchpad->triggered)
		return;
	touchpad->triggered = true;

	modMessagePostToMachineFromISR(touchpad->the, touchpadDeliver, touchpad);
}

void touchpadDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modTouchpad touchpad = refcon;

	if (touchpad != gTouchpad)
		return;

	if (touchpad->closed) {
		xs_touchpad_destructor(touchpad);
		return;
	}

	touchpad->triggered = false;
	
	xsBeginHost(the);
		xsCallFunction0(xsReference(touchpad->onReadable), touchpad->obj);
	xsEndHost(the);
}
