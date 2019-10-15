/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "nrfx_qdec.h"

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	int16_t		acc;			// last read
	int16_t		accdbl;			// last read
uint32_t dblerr;							// was expected ot have 0 but not
	int			base;			// from user
	int			current;		// current value	
	int			changeQueued;
} PulseCountRecord, *PulseCount;

static PulseCount gPCR = NULL;

static void qdec_event_deliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (gPCR) {
		gPCR->changeQueued = 0;
		xsBeginHost(the);
			xsCall0(gPCR->obj, xsID_onChanged);
		xsEndHost(the);
	}
}


static void qdec_event_handler(nrfx_qdec_event_t event)
{
    if (event.type == NRF_QDEC_EVENT_REPORTRDY)
    {
        gPCR->accdbl        = event.data.report.accdbl;
		if (0 != gPCR->accdbl)
			gPCR->dblerr++;
        gPCR->acc           = event.data.report.acc;
		gPCR->current += gPCR->acc;
		if (!gPCR->changeQueued) {
			gPCR->changeQueued = 1;
			modMessagePostToMachineFromISR(gPCR->the, qdec_event_deliver, gPCR);
		}
    }
}


void xs_pulsecount_destructor(void *data)
{
}

void xs_pulsecount(xsMachine *the)
{
	PulseCountRecord pc;
	int signal, control, filter = 100;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_signal);
	signal = xsmcToInteger(xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_control);
	control = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_filter);
		filter = xsmcToInteger(xsVar(0));
	}
	pc.the = the;
	pc.obj = xsThis;
	pc.base = 0;
	pc.acc = 0;
	pc.accdbl = 0;
pc.dblerr = 0;
	pc.current = 0;
	pc.changeQueued = 0;
	xsmcSetHostChunk(xsThis, &pc, sizeof(PulseCountRecord));
	xsRemember(pc.obj);
	gPCR = xsmcGetHostChunk(xsThis);

	nrfx_qdec_config_t qdec_config = {
        .reportper          = (nrf_qdec_reportper_t)NRFX_QDEC_CONFIG_REPORTPER,
        .sampleper          = (nrf_qdec_sampleper_t)NRFX_QDEC_CONFIG_SAMPLEPER,
        .psela              = NRFX_QDEC_CONFIG_PIO_A,
        .pselb              = NRFX_QDEC_CONFIG_PIO_B,
        .pselled            = NRF_QDEC_LED_NOT_CONNECTED,
        .ledpre             = NRFX_QDEC_CONFIG_LEDPRE,
        .ledpol             = (nrf_qdec_ledpol_t)NRFX_QDEC_CONFIG_LEDPOL,
        .interrupt_priority = NRFX_QDEC_CONFIG_IRQ_PRIORITY,
        .dbfen              = NRFX_QDEC_CONFIG_DBFEN,
        .sample_inten       = NRFX_QDEC_CONFIG_SAMPLE_INTEN      
	};

	qdec_config.psela = signal;
	qdec_config.pselb = control;

	int err;
	err = nrfx_qdec_init(&qdec_config, qdec_event_handler);

	nrfx_qdec_enable();
}

void xs_pulsecount_close(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	if (!pc) return;
	nrfx_qdec_uninit();
	xsForget(pc->obj);
gPCR = NULL;
	xs_pulsecount_destructor(pc);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pulsecount_get(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	int16_t count;

	if (!pc) return;

	xsmcSetInteger(xsResult, pc->current + pc->base);
}

void xs_pulsecount_set(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	if (!pc) return;

	pc->base = xsmcToInteger(xsArg(0));
	uint16_t d1, d2;
	nrfx_qdec_accumulators_read(&d1,&d2);
	pc->current = 0;
}
