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

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"
#include "driver/touch_pad.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	uint32_t			pins;
	uint32_t			values;
	double				sensitivity;
    uint8_t             running;
	uint8_t				closed;
	uint8_t				triggered;
} modTouchpadRecord, *modTouchpad;

#define DEFAULT_SENSITIVITY 0.2
#define MAXIMUM_PIN 31

static void setThresholds(uint32_t pins, double sensitivity);
static void touchpadDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void touchpadISR(void *refcon);

static uint8_t gTouchpadInit = 0;

void xs_touchpad_destructor(void *data)
{
	modTouchpad touchpad = data;
	if (NULL == touchpad)
		return;

	touch_pad_fsm_stop();
	touch_pad_isr_deregister(touchpadISR, touchpad);
	touch_pad_deinit();
    gTouchpadInit = 0;
	c_free(touchpad);
}

void xs_touchpad(xsMachine *the)
{
	modTouchpad touchpad;
    int guardPin = -1;
	double sensitivity = DEFAULT_SENSITIVITY;

	if (gTouchpadInit++)
        xsUnknownError("ESP32 only supports one touchpad instance");
    
    xsmcVars(1);

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
	touchpad->pins = 0;
	touchpad->values = 0;
	touchpad->sensitivity = sensitivity;
    touchpad->running = false;
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
}

void xs_touchpad_close(xsMachine *the)
{
	modTouchpad touchpad = xsmcGetHostData(xsThis);

	xsForget(touchpad->obj);
	touchpad->closed = true;
	if (!touchpad->triggered)
		xs_touchpad_destructor(touchpad);
	xsmcSetHostData(xsThis, NULL);
}

void xs_touchpad_add(xsMachine *the)
{
    modTouchpad touchpad = xsmcGetHostData(xsThis);
    int pin;

	xsmcVars(1);

    if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	if (pin > MAXIMUM_PIN)
		xsUnknownError("invalid touch pin");

    touch_pad_config(pin);
	touchpad->pins |= (1 << pin);

	if (!touchpad->running) {
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
		touchpad->running = true;
	}else{
		touch_pad_fsm_stop();
	}

	touch_pad_fsm_start(); // have to restart fsm after a new touch pin is added
	vTaskDelay(100 / portTICK_RATE_MS); //@@ there does seem to need to be a delay between fsm_start and setting thresholds, but this may not be the best way to implement it

	setThresholds(touchpad->pins, touchpad->sensitivity); // have to reset all thresholds after fsm restart
}

void xs_touchpad_remove(xsMachine *the)
{
    modTouchpad touchpad = xsmcGetHostData(xsThis);
    int pin;
	uint32_t value;

	xsmcVars(1);

    if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	if (pin > MAXIMUM_PIN)
		xsUnknownError("invalid touch pin");

	//@@ Touchpad API does not seem to have a way to de-initialize a pad... So, instead set touch threshold to max to prevent interrupts from that pad.
	touch_pad_set_thresh(pin, TOUCH_PAD_THRESHOLD_MAX);

	touchpad->pins &= ~(1 << pin);
}

void xs_touchpad_read(xsMachine *the)
{
    modTouchpad touchpad = xsmcGetHostData(xsThis);
	xsResult = xsInteger(touchpad->values);
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

	if (touchpad->triggered)
			return;
	touchpad->triggered = true;

	modMessagePostToMachineFromISR(touchpad->the, touchpadDeliver, touchpad);
}

void touchpadDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modTouchpad touchpad = refcon;
	if (touchpad->closed) {
		xs_touchpad_destructor(touchpad);
		return;
	}

	touchpad->triggered = false;

	xsBeginHost(the);
		xsmcVars(1);
		xsCall0(touchpad->obj, xsID_onChanged);
	xsEndHost(the);
}
