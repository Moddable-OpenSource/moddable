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

#include "driver/mcpwm.h"

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
    uint8_t     channel;
    uint8_t     edges;
    uint8_t     hasOnReadable;

    // Allocated only if onReadable callback present
	xsMachine	*the;
	xsSlot		*onReadable;
};
typedef struct PulseWidthRecord PulseWidthRecord;
typedef struct PulseWidthRecord *PulseWidth;

static bool pw_callback(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata, void *arg);
static void pulseWidthDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
void xs_pulsewidth_destructor(void *data);
void xs_pulsewidth_mark(xsMachine* the, void *it, xsMarkRoot markRoot);
static uint8_t mcpwmFromChannel(uint8_t channel, mcpwm_unit_t *unit, mcpwm_io_signals_t *signal, mcpwm_capture_signal_t *select);

static uint8_t gFreeMCPWM = 0b00111111;

const xsHostHooks ICACHE_RODATA_ATTR xsPulseWidthHooks = {
	xs_pulsewidth_destructor,
	xs_pulsewidth_mark,
	NULL
};

void xs_pulsewidth_constructor(xsMachine *the)
{
	PulseWidth pw;
	int pin;
	xsSlot *onReadable;
	xsSlot tmp;
    int result;
    uint8_t channel;
    int edges;
    int mode = kPulseWidthFloating;

    mcpwm_unit_t unit;
    mcpwm_io_signals_t signal;
    mcpwm_capture_signal_t select;


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
    
    channel = mcpwmFromChannel(gFreeMCPWM, &unit, &signal, &select);

    if (!channel)
        xsRangeError("all mcpwm channels used");

    pw = c_malloc(onReadable ? sizeof(PulseWidthRecord) : offsetof(PulseWidthRecord, the));
	if (!pw)
		xsRangeError("no memory");

    gFreeMCPWM &= ~channel;

	xsmcSetHostData(xsThis, pw);
	pw->pin = pin;
	pw->obj = xsThis;
    pw->channel = channel;
    pw->edges = edges;
    pw->hasOnReadable = onReadable ? 1 : 0;

    if (onReadable) {
        pw->the = the;
	    pw->onReadable = onReadable;
    }

	xsRemember(pw->obj);    
    
    mcpwm_gpio_init(unit, signal, pin);

    if (mode == kPulseWidthPullDown) {
        gpio_pulldown_en(pin);
    } else if (mode == kPulseWidthPullUp) {
        gpio_pullup_en(pin);
    } else {
        gpio_pulldown_dis(pin);
        gpio_pullup_dis(pin);
    }
    
    mcpwm_capture_config_t conf = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = pw_callback,
        .user_data = pw
    };

    if (edges == kPulseWidthFallingToFalling)
        conf.cap_edge = MCPWM_NEG_EDGE;

    if (edges == kPulseWidthRisingToRising)
        conf.cap_edge = MCPWM_POS_EDGE;

    mcpwm_capture_enable_channel(unit, select, &conf);
    builtinUsePin(pin);

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPulseWidthHooks);
}

void xs_pulsewidth_destructor(void *data)
{
    PulseWidth pw = data;
    mcpwm_unit_t unit;
    mcpwm_io_signals_t signal;
    mcpwm_capture_signal_t select;
    uint8_t test;
    
    if (!pw)
        return;

    gpio_pulldown_dis(pw->pin);
    gpio_pullup_dis(pw->pin);
    
    test = mcpwmFromChannel(pw->channel, &unit, &signal, &select);
    if (test) {
        mcpwm_capture_disable_channel(unit, select);
        gFreeMCPWM |= test;
    }

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

static bool pw_callback(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata, void *arg)
{
    static uint32_t cap_val_begin_of_sample = 0;
    PulseWidth pw = (PulseWidth)arg;

    if (edata->cap_edge == MCPWM_POS_EDGE) {
        switch (pw->edges) {
            case kPulseWidthRisingToFalling:
                cap_val_begin_of_sample = edata->cap_value;
                break;
            case kPulseWidthFallingToRising:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                    cap_val_begin_of_sample = 0;
                    if (pw->hasOnReadable)
                        modMessagePostToMachineFromISR(pw->the, pulseWidthDeliver, pw);
                }
                break;
            case kPulseWidthRisingToRising:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                    if (pw->hasOnReadable)
                        modMessagePostToMachineFromISR(pw->the, pulseWidthDeliver, pw);
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
                    if (pw->hasOnReadable)
                        modMessagePostToMachineFromISR(pw->the, pulseWidthDeliver, pw);
                }
                break;
            case kPulseWidthFallingToFalling:
                if (cap_val_begin_of_sample) {
                    pw->value = edata->cap_value - cap_val_begin_of_sample;
                    if (pw->hasOnReadable)
                        modMessagePostToMachineFromISR(pw->the, pulseWidthDeliver, pw);
                }
                cap_val_begin_of_sample = edata->cap_value;
                break;
        }
    }

    return false;
}

static void pulseWidthDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	PulseWidth pw = refcon;

	xsBeginHost(pw->the);
		xsCallFunction0(xsReference(pw->onReadable), pw->obj);
	xsEndHost(pw->the);
}

static uint8_t mcpwmFromChannel(uint8_t channel, mcpwm_unit_t *unit, mcpwm_io_signals_t *signal, mcpwm_capture_signal_t *select)
{
    uint8_t x = channel;
    uint8_t result;

    if (x == 0)
        return 0;

    if (x & 0b000111) {
        *unit = MCPWM_UNIT_0;
    } else {
        x >>= 3;
        *unit = MCPWM_UNIT_1;
    }

    if (x & 0b1) {
        *signal = MCPWM_CAP_0;
        *select = MCPWM_SELECT_CAP0;
        result = 0b1;
    } else if (x & 0b10) {
        *signal = MCPWM_CAP_1;
        *select = MCPWM_SELECT_CAP1;
        result = 0b10;
    } else if (x & 0b100) {
        *signal = MCPWM_CAP_2;
        *select = MCPWM_SELECT_CAP2;
        result = 0b100;
    }

    if (*unit == MCPWM_UNIT_1)
        result <<= 3;

    return result;
}

#else // ! SOC_MCPWM_SUPPORTED
void xs_pulsewidth_read(xsMachine *the) {}
void xs_pulsewidth_mark(xsMachine* the, void *it, xsMarkRoot markRoot) {}
void xs_pulsewidth_destructor(void *data) {}
void xs_pulsewidth_constructor(xsMachine *the) {}
void xs_pulsewidth_close(xsMachine *the) {}
#endif