/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

/*
	to do:
	
		- useCount to defend against being closed with messages in flight
*/

#include "xsmc.h"           // xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"         // platform support

#if defined(SOC_PCNT_SUPPORTED)
#include "builtinCommon.h"

#include "driver/pulse_cnt.h"

#define PCNT_PIN_NOT_USED (-1)

struct PulseCountRecord {
	int						base;
    int32_t					signal;
    int32_t					control;
    xsSlot					obj;
	uint8_t					hasOnReadable;
	uint8_t					closing;
    pcnt_channel_handle_t	channelA;
    pcnt_channel_handle_t	channelB;
    pcnt_unit_handle_t		unit;
	// fields after here only allocated if onReadable callback present
	uint8_t					triggered;
    xsMachine				*the;
    xsSlot					*onReadable;
	struct PulseCountRecord	*next;
};
typedef struct PulseCountRecord PulseCountRecord;
typedef struct PulseCountRecord *PulseCount;

static bool on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);
static void pulsecountDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_pulsecount_mark_(xsMachine* the, void *it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsPulseCountHooks = {
	xs_pulsecount_destructor_,
	xs_pulsecount_mark_,
	NULL
};

void xs_pulsecount_destructor_(void *data)
{
    PulseCount pc = data;

    if (!pc)
        return;

	builtinFreePin(pc->signal);
    if (pc->control != PCNT_PIN_NOT_USED)
        builtinFreePin(pc->control);

	if (pc->channelB)
		pcnt_del_channel(pc->channelB);
	if (pc->channelA)
		pcnt_del_channel(pc->channelA);
	if (pc->unit) {
		pcnt_unit_disable(pc->unit);
		pcnt_del_unit(pc->unit);
	}

    c_free(pc);
}

void xs_pulsecount_constructor_(xsMachine *the)
{
	esp_err_t err;
	PulseCount pc;
	int signal, control = PCNT_PIN_NOT_USED, filter = 1000;
    xsSlot *onReadable;
	xsmcVars(1);
	
	xsmcGet(xsVar(0), xsArg(0), xsID_signal);
	signal = builtinGetPin(the, &xsVar(0));
	if (!builtinIsPinFree(signal))
		xsRangeError("in use");

	if (xsmcHas(xsArg(0), xsID_control)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_control);
		control = builtinGetPin(the, &xsVar(0));

        if (!builtinIsPinFree(control))
		    xsRangeError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_filter);
		filter = xsmcToInteger(xsVar(0));
	}

    pcnt_unit_config_t unit_config = {
		.high_limit = +32767,
		.low_limit = -32768,
    };
    pcnt_unit_handle_t unit;
    err = pcnt_new_unit(&unit_config, &unit);
    if (err)
		xsUnknownError("no pcnt");

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = filter
    };
    pcnt_unit_set_glitch_filter(unit, &filter_config);

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = signal,
        .level_gpio_num = control
    };
    pcnt_channel_handle_t channelA;
    err = pcnt_new_channel(unit, &chan_a_config, &channelA);
	if (err) {
		pcnt_del_unit(unit);
		xsUnknownError("no pcnt a channel");
	}

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = control,
        .level_gpio_num = signal
    };
    pcnt_channel_handle_t channelB;
    err = pcnt_new_channel(unit, &chan_b_config, &channelB);
	if (err) {
		pcnt_del_channel(channelA);
		pcnt_del_unit(unit);
		xsUnknownError("no pcnt b channel");
	}

	onReadable = builtinGetCallback(the, xsID_onReadable);
    pc = c_malloc(onReadable ? sizeof(PulseCountRecord) : offsetof(PulseCountRecord, triggered));
    if (!pc) {
		pcnt_del_channel(channelB);
		pcnt_del_channel(channelA);
		pcnt_del_unit(unit);
        xsRangeError("no memory");
	}

    pc->obj = xsThis;
    xsRemember(pc->obj);
    xsmcSetHostData(xsThis, pc);

	pc->base = 0;
    pc->signal = signal;
    pc->control = control;
    pc->hasOnReadable = onReadable ? 1 : 0;
    pc->unit = unit;
    pc->channelA = channelA;
    pc->channelB = channelB;
    pc->closing = 0;

    builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	pcnt_channel_set_edge_action(channelA, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
	pcnt_channel_set_level_action(channelA, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
	pcnt_channel_set_edge_action(channelB, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
	pcnt_channel_set_level_action(channelB, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

	if (onReadable) {
		pc->triggered = 0;
		pc->the = the;
		pc->onReadable = onReadable;

		pcnt_unit_add_watch_point(unit, -1);
		pcnt_unit_add_watch_point(unit, +1);

		pcnt_event_callbacks_t cbs = {
			.on_reach = on_reach
		};
		pcnt_unit_register_event_callbacks(unit, &cbs, pc);
    }

	pcnt_unit_enable(unit);
	pcnt_unit_clear_count(unit);
	pcnt_unit_start(unit);

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPulseCountHooks);

    builtinUsePin(signal);
    if (PCNT_PIN_NOT_USED != control)
        builtinUsePin(control);
}

void xs_pulsecount_mark_(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	PulseCount pc = it;
	
	if (pc->hasOnReadable)
		(*markRoot)(the, pc->onReadable);
}

void xs_pulsecount_close_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostData(xsThis);
	if (pc && xsmcGetHostDataValidate(xsThis, (void *)&xsPulseCountHooks)) {
		pc->closing = 1;
		xsForget(pc->obj);
		xs_pulsecount_destructor_(pc);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_pulsecount_read_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, (void *)&xsPulseCountHooks);
	int count;

	pcnt_unit_get_count(pc->unit, &count);
	pcnt_unit_clear_count(pc->unit);
	pc->base += count;
	xsmcSetInteger(xsResult, pc->base);
}

void xs_pulsecount_write_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, (void *)&xsPulseCountHooks);

	pc->base = xsmcToInteger(xsArg(0));
	pcnt_unit_clear_count(pc->unit);
}

bool IRAM_ATTR on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *refcon)
{
	PulseCount pc = (PulseCount)refcon;

	if (!pc->triggered && !pc->closing) {
		pc->triggered = 1;
		modMessagePostToMachineFromISR(pc->the, pulsecountDeliver, pc);
	}

	return false;
}

void pulsecountDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	PulseCount pc = refcon;
	pc->triggered = 0;

	xsBeginHost(pc->the);
		xsCallFunction0(xsReference(pc->onReadable), pc->obj);
	xsEndHost(pc->the);
}

#else 		//  ! SOC_PCNT_SUPPORTED
void xs_pulsecount_destructor_(void *data) {}
void xs_pulsecount_constructor_(xsMachine *the) {
	xsUnknownError("no hardware");
}
void xs_pulsecount_mark_(xsMachine* the, void *it, xsMarkRoot markRoot) {}
void xs_pulsecount_close_(xsMachine *the) {}
void xs_pulsecount_read_(xsMachine *the) {}
void xs_pulsecount_write_(xsMachine *the) {}
#endif
