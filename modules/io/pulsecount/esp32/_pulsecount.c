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

#include "xsmc.h"           // xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"         // platform support

#if defined(SOC_PCNT_SUPPORTED)
#include "builtinCommon.h"

#include "driver/pcnt.h"
#include "soc/pcnt_struct.h"

struct PulseCountRecord {
	int			unit;
	int			base;
    int32_t     signal;
    int32_t     control;
    xsSlot      obj;
	uint8_t		hasOnReadable;
	// fields after here only allocated if onReadable callback present
	uint8_t		triggered;
    xsMachine   *the;
    xsSlot      *onReadable;
	struct PulseCountRecord *next;
};
typedef struct PulseCountRecord PulseCountRecord;
typedef struct PulseCountRecord *PulseCount;

#define PCNT_AVAILABLE (SOC_PCNT_GROUPS * SOC_PCNT_UNITS_PER_GROUP)

static uint8_t gPulseCountUnitsAvailable = PCNT_AVAILABLE;

static PulseCount gPulseCounts; // pulse count units with onReadable callbacks
static pcnt_isr_handle_t gPulseCountISRHandle;

static void pulsecountISR(void *refcon);
void pulsecountDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
void xs_pulsecount_mark_(xsMachine* the, void *it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsPulseCountHooks = {
	xs_pulsecount_destructor_,
	xs_pulsecount_mark_,
	NULL
};

void xs_pulsecount_destructor_(void *data)
{
    PulseCount pc = data;
	uint8_t hadInterrupts = 0;

    if (!pc)
        return;

	//@@ check to see if critical section can be shortened
	builtinCriticalSectionBegin();

	if (gPulseCounts == pc) {
		gPulseCounts = pc->next;
		hadInterrupts = 1;
	} else {
		PulseCount walker;
		for (walker = gPulseCounts; walker; walker = walker->next) {
			if (walker->next == pc) {
				walker->next = pc->next;
				hadInterrupts = 1;
				break;
			}
		}
	}

    gPulseCountUnitsAvailable |= (1 << pc->unit);
	builtinFreePin(pc->signal);
    if (pc->control != PCNT_PIN_NOT_USED)
        builtinFreePin(pc->control);

	pcnt_counter_pause(pc->unit);
	pcnt_counter_clear(pc->unit);

	if (hadInterrupts) {
		pcnt_intr_disable(pc->unit);
		pcnt_event_disable(pc->unit, PCNT_EVT_THRES_0);
		pcnt_event_disable(pc->unit, PCNT_EVT_THRES_1);
		
		if (gPulseCounts == NULL && gPulseCountISRHandle != NULL) {
			//pcnt_isr_unregister(gPulseCountISRHandle); // @@ should be necessary, but seems to hang...
			gPulseCountISRHandle = NULL;
		}
			
	}

	builtinCriticalSectionEnd();

    c_free(pc);
}

void xs_pulsecount_constructor_(xsMachine *the)
{
	PulseCount pc;
	int signal, control, unit, filter = 100;
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
	} else {
		control = PCNT_PIN_NOT_USED;
	}

    if (gPulseCountUnitsAvailable == 0)
        xsRangeError("all pulsecount units are in use");

    for (int i = PCNT_UNIT_0; i < PCNT_UNIT_MAX; i++) {
        if ( (gPulseCountUnitsAvailable >> i) & 1) {
            unit = i;
            gPulseCountUnitsAvailable &= ~(1 << i);
			break;
        }	
    }

	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_filter);
		filter = xsmcToInteger(xsVar(0));
	}

	onReadable = builtinGetCallback(the, xsID_onReadable);
    pc = c_malloc(onReadable ? sizeof(PulseCountRecord) : offsetof(PulseCountRecord, triggered));
    if (!pc)
        xsRangeError("no memory");

    pc->obj = xsThis;
    xsRemember(pc->obj);
    xsmcSetHostData(xsThis, pc);

	pc->base = 0;
    pc->unit = unit;
    pc->signal = signal;
    pc->control = control;
    pc->hasOnReadable = onReadable ? 1 : 0;

    builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	pcnt_config_t pcnt_config = {
		.pulse_gpio_num = 0,
		.ctrl_gpio_num = 0,
		.channel = PCNT_CHANNEL_0,
		.unit = 0,
		.pos_mode = PCNT_COUNT_INC,
		.neg_mode = PCNT_COUNT_DIS,
		.lctrl_mode = PCNT_MODE_REVERSE,
		.hctrl_mode = PCNT_MODE_KEEP,
		.counter_h_lim = 32767,
		.counter_l_lim = -32768
	};

	pcnt_config.pulse_gpio_num = signal;
	pcnt_config.ctrl_gpio_num = control;
	pcnt_config.unit = pc->unit;

	pcnt_unit_config(&pcnt_config);

	pcnt_set_filter_value(pc->unit, filter);
	pcnt_filter_enable(pc->unit);

	if (onReadable) {
		xsSlot tmp;

		pc->triggered = 0;
		pc->the = the;
		pc->onReadable = onReadable;

		if (NULL == gPulseCounts)
			pcnt_isr_register(pulsecountISR, (void *)(uintptr_t)pc->unit, 0, &gPulseCountISRHandle);

		builtinCriticalSectionBegin();
			pc->next = gPulseCounts;
			gPulseCounts = pc;
		builtinCriticalSectionEnd();

		pcnt_set_event_value(pc->unit, PCNT_EVT_THRES_0, -1);
		pcnt_set_event_value(pc->unit, PCNT_EVT_THRES_1, 1);
		pcnt_intr_enable(pc->unit);
		pcnt_event_enable(pc->unit, PCNT_EVT_THRES_0);
		pcnt_event_enable(pc->unit, PCNT_EVT_THRES_1);
    }
	
	pcnt_counter_pause(pc->unit);
	pcnt_counter_clear(pc->unit);
	pcnt_counter_resume(pc->unit);

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPulseCountHooks);

    builtinUsePin(signal);
    if (control != PCNT_PIN_NOT_USED)
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
		xsForget(pc->obj);
		xs_pulsecount_destructor_(pc);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_pulsecount_read_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, (void *)&xsPulseCountHooks);
	int16_t count;

	pcnt_get_counter_value(pc->unit, &count);
	xsmcSetInteger(xsResult, count + pc->base);
}

void xs_pulsecount_write_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, (void *)&xsPulseCountHooks);

	pc->base = xsmcToInteger(xsArg(0));
	pcnt_counter_clear(pc->unit);
}

void IRAM_ATTR pulsecountISR(void *refcon)
{
	uint32_t unit = (uintptr_t)refcon;
	PulseCount walker;

	for (walker = gPulseCounts; walker; walker = walker->next) {
		if (unit != walker->unit)
			continue;

		int16_t count;
		builtinCriticalSectionBegin();
			pcnt_get_counter_value(walker->unit, &count);
			pcnt_counter_clear(walker->unit);
			PCNT.int_clr.val = 0xFF;
			walker->base += count;
		builtinCriticalSectionEnd();

		if (!walker->triggered) {
			walker->triggered = 1;
			modMessagePostToMachineFromISR(walker->the, pulsecountDeliver, walker);
		}
		break;
	}

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
void xs_pulsecount_constructor_(xsMachine *the) {}
void xs_pulsecount_mark_(xsMachine* the, void *it, xsMarkRoot markRoot) {}
void xs_pulsecount_close_(xsMachine *the) {}
void xs_pulsecount_read_(xsMachine *the) {}
void xs_pulsecount_write_(xsMachine *the) {}
#endif
