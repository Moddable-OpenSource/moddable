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
#include "xsHost.h"
#include "mc.defines.h"
#include "em_adc.h"
#include "em_gpio.h"

uint32_t adcSingle(bool ovs, uint32_t inputChan, uint32_t reference);
uint32_t ADC_Calibration(ADC_TypeDef *adc, ADC_Ref_TypeDef ref);
void adcSetup();
void adcTerminate();

static void calibrate() {
	ADC_TypeDef *adc;
	ADC_Ref_TypeDef ref;
#ifdef MODDEF_ANALOG_PORT
	adc = MODDEF_ANALOG_PORT;
#else
	adc = ADC0;
#endif
#ifdef MODDEF_ANALOG_REF
	ref = MODDEF_ANALOG_REF;
#else
	ref = adcRefVDD;
#endif
	ADC_Calibration(adc, ref);
}

void xs_analog_read(xsMachine *the) {
	int chan = xsToInteger(xsArg(0));
	uint32_t ret, inputChan = 0;
	int gotChan = 0;

	adcSetup();

	switch (chan) {
#ifdef MODDEF_ANALOG_INPUT1
		case 1:
			inputChan = MODDEF_ANALOG_INPUT1;
			gotChan = 1;
			break;
#endif
#ifdef MODDEF_ANALOG_INPUT2
		case 2:
			inputChan = MODDEF_ANALOG_INPUT2;
			gotChan = 1;
			break;
#endif
#ifdef MODDEF_ANALOG_INPUT3
		case 3:
			inputChan = MODDEF_ANALOG_INPUT3;
			gotChan = 1;
			break;
#endif
#ifdef MODDEF_ANALOG_INPUT4
		case 4:
			inputChan = MODDEF_ANALOG_INPUT4;
			gotChan = 1;
			break;
#endif
#ifdef MODDEF_ANALOG_INPUT5
		case 5:
			inputChan = MODDEF_ANALOG_INPUT5;
			gotChan = 1;
			break;
#endif
		default:
			xsUnknownError("bad analog input # " + chan);
	}
	if (gotChan) {
#ifdef MODDEF_ANALOG_REF
		ret = adcSingle(true, inputChan, MODDEF_ANALOG_REF);
#else
		ret = adcSingle(true, inputChan, adcRefVDD);
#endif
	}

	xsResult = xsInteger(ret);

	adcTerminate();
}

