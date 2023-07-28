/*
 * Copyright (c) 2019-2023  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"
#include "builtinCommon.h"
#include "nrfx_qdec.h"
#include "nrf_gpio.h"

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	xsSlot		onReadable;
	int32_t		current;		// current value
	uint8_t		changeQueued;
	uint8_t		hasOnReadable;
	uint8_t		signal;
	uint8_t		control;
} PulseCountRecord, *PulseCount;

static PulseCount gPCR = NULL;

static void qdec_event_deliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gPCR)
		return;

	gPCR->changeQueued = 0;
	xsBeginHost(the);
		xsCallFunction0(gPCR->onReadable, gPCR->obj);
	xsEndHost(the);
}

static void qdec_event_handler(nrfx_qdec_event_t event)
{
    if (event.type == NRF_QDEC_EVENT_REPORTRDY) {
		gPCR->current += event.data.report.acc;
		if (!gPCR->changeQueued && gPCR->hasOnReadable) {
			gPCR->changeQueued = 1;
			modMessagePostToMachineFromISR(gPCR->the, qdec_event_deliver, NULL);
		}
    }
}

void xs_pulsecount_destructor_(void *data)
{
	PulseCount pc = data;
	if (pc) {
		nrfx_qdec_uninit();
		builtinFreePin(pc->signal);
		builtinFreePin(pc->control);
		gPCR = NULL;
		c_free(data);
	}
}

void xs_pulsecount_(xsMachine *the)
{
	PulseCount pc;
	int signal, control;
	xsSlot *onReadable;

	 if (gPCR)
	 	xsUnknownError("busy");

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_signal);
	signal = builtinGetPin(the, &xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_control);
	control = builtinGetPin(the, &xsVar(0));

	if (!builtinIsPinFree(signal) || !builtinIsPinFree(control))
		xsRangeError("inUse");

	builtinInitializeTarget(the);
	onReadable = builtinGetCallback(the, xsID_onReadable);

	pc = c_malloc(sizeof(PulseCountRecord));
	if (!pc)
		xsUnknownError("no memory");

	nrfx_qdec_config_t qdec_config = {
        .reportper          = (nrf_qdec_reportper_t)NRF_QDEC_REPORTPER_10,
        .sampleper          = (nrf_qdec_sampleper_t)QDEC_SAMPLEPER_SAMPLEPER_2048us,
        .psela              = signal,
        .pselb              = control,
        .pselled            = NRF_QDEC_LED_NOT_CONNECTED,
        .ledpre             = NRFX_QDEC_CONFIG_LEDPRE,
        .ledpol             = (nrf_qdec_ledpol_t)NRFX_QDEC_CONFIG_LEDPOL,
        .interrupt_priority = _PRIO_APP_LOW,	// 6 - NRFX_QDEC_CONFIG_IRQ_PRIORITY,
        .dbfen              = 1,		// debounce enabled
        .sample_inten       = NRFX_QDEC_CONFIG_SAMPLE_INTEN      
	};

	if (NRFX_SUCCESS != nrfx_qdec_init(&qdec_config, qdec_event_handler)) {
		c_free(pc);
		xsUnknownError("qdec init failed");
	}

	pc->the = the;
	pc->obj = xsThis;
	xsRemember(pc->obj);

	builtinUsePin(signal);
	builtinUsePin(control);
	pc->signal = (uint8_t)signal;
	pc->control = (uint8_t)control;

	pc->hasOnReadable = onReadable ? true : false;
	if (onReadable) {
		pc->onReadable = xsReference(onReadable);
		xsRemember(pc->onReadable);
	}

	pc->current = 0;
	pc->changeQueued = 0;
	xsmcSetHostData(xsThis, pc);
	gPCR = pc;

    nrf_gpio_cfg_input(signal, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(control, NRF_GPIO_PIN_PULLUP);

	nrfx_qdec_enable();
}

void xs_pulsecount_close_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostData(xsThis);
	if (!pc) return;
	pc = xsmcGetHostDataValidate(xsThis, xs_pulsecount_destructor_);
	xsForget(pc->obj);
	if (pc->hasOnReadable)
		xsForget(pc->onReadable);
	xs_pulsecount_destructor_(pc);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pulsecount_read_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, xs_pulsecount_destructor_);
	xsmcSetInteger(xsResult, pc->current);
}

void xs_pulsecount_write_(xsMachine *the)
{
	PulseCount pc = xsmcGetHostDataValidate(xsThis, xs_pulsecount_destructor_);
	pc->current = xsmcToInteger(xsArg(0));
}
