/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xs.h"
#include "xsgecko.h"
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

void xs_Analog(xsMachine *the) {
	adcSetup();
#ifdef MODDEF_ANALOG_POWER_PIN
	GPIO_PinModeSet(MODDEF_ANALOG_POWER_PORT, MODDEF_ANALOG_POWER_PIN, gpioModePushPull, 0);
#endif
//	calibrate();
}

void xs_Analog_destructor(void *data) {
	adcTerminate();
#ifdef MODDEF_ANALOG_POWER_PIN
	GPIO_PinOutClear( MODDEF_ANALOG_POWER_PORT, MODDEF_ANALOG_POWER_PIN );
	GPIO_PinModeSet(MODDEF_ANALOG_POWER_PORT, MODDEF_ANALOG_POWER_PIN, gpioModeDisabled, 0);
#endif
}

void xs_Analog_sleepEM4() {
	xs_Analog_destructor(NULL);
}


void xs_Analog_read(xsMachine *the) {
	uint32_t chan = xsToInteger(xsArg(0));
	uint32_t ret, inputChan = 0;
	int gotChan = 0;
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
		GPIO_PinOutSet( MODDEF_ANALOG_POWER_PORT, MODDEF_ANALOG_POWER_PIN );
#ifdef MODDEF_ANALOG_REF
		ret = adcSingle(true, inputChan, MODDEF_ANALOG_REF);
#else
		ret = adcSingle(true, inputChan, adcRefVDD);
#endif
		GPIO_PinOutClear( MODDEF_ANALOG_POWER_PORT, MODDEF_ANALOG_POWER_PIN );
	}

	xsResult = xsInteger(ret);
}

