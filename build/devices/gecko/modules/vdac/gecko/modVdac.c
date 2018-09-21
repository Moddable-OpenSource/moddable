/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include "xs.h"
#include "xsgecko.h"
#include "mc.defines.h"
#include "em_vdac.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define VDAC_RESOLUTION	4095

#if !defined(MODDEF_VDAC_INTERFACE_VDAC)
	#define MODDEF_VDAC_INTERFACE_VDAC	0
#endif
#if !defined(MODDEF_VDAC_AREF_SELECTION)
	#define MODDEF_VDAC_AREF_SELECTION	vdacRef2V5Ln
#endif

#if MODDEF_VDAC_INTERFACE_VDAC == 0
	#define VDAC_PORT	VDAC0
	#define VDAC_CLOCK	cmuClock_VDAC0
#elif MODDEF_VDAC_INTERFACE_VDAC == 1
	#define VDAC_PORT	VDAC1
	#define VDAC_CLOCK	cmuClock_VDAC1
#endif

VDAC_InitChannel_TypeDef vdacChInit[2];
static uint8_t vdacEnabled = 0;

void vdacTerminate();

void vdacSetup() {
	VDAC_Init_TypeDef vdacInit = VDAC_INIT_DEFAULT;

	CMU_ClockEnable(VDAC_CLOCK, true);

	// run in EM2/3
	vdacInit.asyncClockMode = true;

	// set prescaler to get 1 MHz VDAC clock freq
	vdacInit.prescaler = VDAC_PrescaleCalc(1000000, true, 0);
	vdacInit.reference = MODDEF_VDAC_AREF_SELECTION;
	VDAC_Init(VDAC_PORT, &vdacInit);

#if defined(MODDEF_VDAC_AREF_PIN)
		// need to set up the aref pin
#endif

#if defined(MODDEF_VDAC_AOUT1_PIN)
	VDAC_PORT->OPA[0].TIMER = 0;
	vdacChInit[0].enable = true;
	vdacChInit[0].prsSel = vdacPrsSelCh0;
	vdacChInit[0].prsAsync = false;
	vdacChInit[0].trigMode = vdacTrigModeSw;
	vdacChInit[0].sampleOffMode = false;
	VDAC_InitChannel(VDAC_PORT, &vdacChInit[0],  0);
#endif
#if defined(MODDEF_VDAC_AOUT2_PIN)
	VDAC_PORT->OPA[1].TIMER = 0;
	vdacChInit[1].enable = true;
	vdacChInit[1].prsSel = vdacPrsSelCh0;
	vdacChInit[1].prsAsync = false;
	vdacChInit[1].trigMode = vdacTrigModeSw;
	vdacChInit[1].sampleOffMode = false;
	VDAC_InitChannel(VDAC_PORT, &vdacChInit[1],  1);
#endif

	vdacEnabled = 1;
}

void xs_vdac_write(xsMachine *the) {
	int inputChan, chan = xsToInteger(xsArg(0));
	int value = xsToInteger(xsArg(1));

	if (!vdacEnabled)
		vdacSetup();

	inputChan = chan - 1;
	if ((inputChan < 0) || (inputChan > 1)) {
		xsUnknownError("bad analog input # " + chan);
	}

	VDAC_ChannelOutputSet(VDAC_PORT, inputChan, value);
}

