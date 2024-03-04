/*
 * Copyright (c) 2022-2023 Moddable Tech, Inc.
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


#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#if defined(SOC_MCPWM_SUPPORTED)
#include "builtinCommon.h"

#include "driver/mcpwm_prelude.h"

enum {
    kPulseWidthRisingToFalling = 1,
    kPulseWidthFallingToRising = 2,
    kPulseWidthRisingToRising = 3,
    kPulseWidthFallingToFalling = 4
};

enum {
    kPulseWidthFloating = 0,
    kPulseWidthPullUp = 1,
    kPulseWidthPullDown = 2
};

struct PulseWidthRecord {
	uint32_t	pin;
	xsSlot		obj;
	uint32_t    value;
    uint8_t     edges;
    uint8_t     hasOnReadable;
	mcpwm_cap_timer_handle_t cap_timer;
	mcpwm_cap_channel_handle_t cap_chan;

    // Allocated only if onReadable callback present
	xsMachine	*the;
	xsSlot		*onReadable;
};
typedef struct PulseWidthRecord PulseWidthRecord;
typedef struct PulseWidthRecord *PulseWidth;

static bool pw_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *arg);
static void pulseWidthDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
void xs_pulsewidth_destructor(void *data);
void xs_pulsewidth_mark(xsMachine* the, void *it, xsMarkRoot markRoot);

const xsHostHooks ICACHE_RODATA_ATTR xsPulseWidthHooks = {
	xs_pulsewidth_destructor,
	xs_pulsewidth_mark,
	NULL
};

void xs_pulsewidth_constructor(xsMachine *the)
{
    esp_err_t err;
	PulseWidth pw;
	int pin;
	xsSlot *onReadable;
	xsSlot tmp;
    int result;
    int edges;
    int mode = kPulseWidthFloating;

    xsmcVars(1);

    if (!xsmcHas(xsArg(0), xsID_pin))
		xsRangeError("pin required");

    if (!xsmcHas(xsArg(0), xsID_edges))
        xsRangeError("edges required");

    xsmcGet(xsVar(0), xsArg(0), xsID_edges);
	edges = xsmcToInteger(xsVar(0));

    xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

    if (xsmcHas(xsArg(0), xsID_mode)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_mode);
	    mode = xsmcToInteger(xsVar(0));
    }

	if (!builtinIsPinFree(pin))
        xsRangeError("in use");
    
    onReadable = builtinGetCallback(the, xsID_onReadable);
    
	builtinInitializeTarget(the);

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");
    
    mcpwm_cap_timer_handle_t cap_timer;
    mcpwm_cap_channel_handle_t cap_chan;

    mcpwm_capture_timer_config_t cap_conf = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };
    err = mcpwm_new_capture_timer(&cap_conf, &cap_timer);
    if (err) 
		xsUnknownError("no capture timer");

    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = pin,
        .prescale = 1,
        // capture on both edge
        .flags.neg_edge = edges != kPulseWidthRisingToRising,
        .flags.pos_edge = edges != kPulseWidthFallingToFalling,
        // pull up internally
        .flags.pull_up = mode == kPulseWidthPullUp,
        .flags.pull_down = mode == kPulseWidthPullDown,
    };

    err = mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan);
    if (err) {
		mcpwm_del_capture_timer(cap_timer);
		xsUnknownError("no capture channel");
	}

    pw = c_malloc(onReadable ? sizeof(PulseWidthRecord) : offsetof(PulseWidthRecord, the));
	if (!pw) {
		mcpwm_del_capture_channel(cap_chan);
		mcpwm_del_capture_timer(cap_timer);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, pw);
	pw->cap_timer = cap_timer;
	pw->cap_chan = cap_chan;
	pw->pin = pin;
	pw->obj = xsThis;
    pw->edges = (uint8_t)edges;
    pw->hasOnReadable = onReadable ? 1 : 0;

    if (onReadable) {
        pw->the = the;
	    pw->onReadable = onReadable;
    }

    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = pw_callback,
    };
	mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, pw);
	mcpwm_capture_channel_enable(cap_chan);
	mcpwm_capture_timer_enable(cap_timer);
	mcpwm_capture_timer_start(cap_timer);

	xsRemember(pw->obj);    

    builtinUsePin(pin);

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPulseWidthHooks);
}

void xs_pulsewidth_destructor(void *data)
{
    PulseWidth pw = data;
    
    if (!pw)
        return;

	if (pw->cap_chan) {
		mcpwm_capture_channel_disable(pw->cap_chan);
		mcpwm_del_capture_channel(pw->cap_chan);
	}

	if (pw->cap_timer) {
		mcpwm_capture_timer_disable(pw->cap_timer);
		mcpwm_del_capture_timer(pw->cap_timer);
	}

    gpio_pulldown_dis(pw->pin);
    gpio_pullup_dis(pw->pin);

    builtinFreePin(pw->pin);
    c_free(pw);
}

void xs_pulsewidth_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	PulseWidth pw = it;

    if (pw->hasOnReadable)
	    (*markRoot)(the, pw->onReadable);
}

void xs_pulsewidth_read(xsMachine *the)
{
    PulseWidth pw = xsmcGetHostDataValidate(xsThis, (void *)&xsPulseWidthHooks);

    if (!pw->value)
        return;

    float pulse_width_us = pw->value * (1000000.0 / rtc_clk_apb_freq_get());
    pw->value = 0;

    xsmcSetNumber(xsResult, pulse_width_us);
}

void xs_pulsewidth_close(xsMachine *the)
{
    PulseWidth pw = xsmcGetHostData(xsThis);

    if (pw && xsmcGetHostDataValidate(xsThis, (void *)&xsPulseWidthHooks)) {
        xsForget(pw->obj);
        xs_pulsewidth_destructor(pw);
        xsmcSetHostData(xsThis, NULL);
        xsmcSetHostDestructor(xsThis, NULL);   
    }
}

static bool pw_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *arg)
{
    static uint32_t cap_val_begin_of_sample = 0;
    PulseWidth pw = (PulseWidth)arg;
    uint8_t ready = 0 != cap_val_begin_of_sample;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        switch (pw->edges) {
            case kPulseWidthRisingToFalling:
                cap_val_begin_of_sample = edata->cap_value;
                break;
            case kPulseWidthFallingToRising:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                    cap_val_begin_of_sample = 0;
                }
                break;
            case kPulseWidthRisingToRising:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                }
                cap_val_begin_of_sample = edata->cap_value;
                break;
        }
    } else {
        switch (pw->edges) {
            case kPulseWidthFallingToRising:
                cap_val_begin_of_sample = edata->cap_value;
                break;
            case kPulseWidthRisingToFalling:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                    cap_val_begin_of_sample = 0;
                }
                break;
            case kPulseWidthFallingToFalling:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                }
                cap_val_begin_of_sample = edata->cap_value;
                break;
        }
    }

	if (ready && pw->hasOnReadable)
		modMessagePostToMachineFromISR(pw->the, pulseWidthDeliver, pw);

    return false;
}

static void pulseWidthDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	PulseWidth pw = refcon;

	xsBeginHost(pw->the);
		xsCallFunction0(xsReference(pw->onReadable), pw->obj);
	xsEndHost(pw->the);
}

#else // ! SOC_MCPWM_SUPPORTED
void xs_pulsewidth_read(xsMachine *the) {}
void xs_pulsewidth_mark(xsMachine* the, void *it, xsMarkRoot markRoot) {}
void xs_pulsewidth_destructor(void *data) {}
void xs_pulsewidth_constructor(xsMachine *the) {
	xsUnknownError("no hardware");
}
void xs_pulsewidth_close(xsMachine *the) {}
#endif
