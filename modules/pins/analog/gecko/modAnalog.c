/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "mc.xs.h"
#include "mc.defines.h"
#include "em_adc.h"
#include "em_gpio.h"

typedef struct modAnalogConfigurationRecord modAnalogConfigurationRecord;
typedef struct modAnalogConfigurationRecord *modAnalogConfiguration;

struct modAnalogConfigurationRecord {
	uint32_t channel;
};

static uint32_t read_analog(uint32_t channel);
static int adc_channel(uint32_t channel);

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

void xs_analog(xsMachine *the)
{
	modAnalogConfiguration analog;
	int channel;
	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);

	channel = adc_channel(xsmcToInteger(xsVar(0)));
	if (channel < 0)
		xsUnknownError("bad analog input #%d", channel);

	analog = c_malloc(sizeof(modAnalogConfigurationRecord));
	if (NULL == analog)
		xsUnknownError("out of memory");
		
	analog->channel = channel;
	xsmcSetHostData(xsThis, analog);

	adcSetup();
}

void xs_analog_destructor(void *data)
{
	modAnalogConfiguration analog = data;
	if (analog) {
		adcTerminate();
		c_free(analog);
	}
}

void xs_analog_close(xsMachine *the)
{
	modAnalogConfiguration analog = (modAnalogConfiguration)xsmcGetHostData(xsThis);
	if (!analog) return;

	xs_analog_destructor(analog);
	xsmcSetHostData(xsThis, NULL);
}

void xs_analog_read(xsMachine *the) {
	modAnalogConfiguration analog = (modAnalogConfiguration)xsmcGetHostChunk(xsThis);
	uint32_t ret;
	
	ret = read_analog(analog->channel);

	xsResult = xsInteger(ret);
}

void xs_analog_static_read(xsMachine *the) {
	uint32_t ret, chan = xsmcToInteger(xsArg(0));
	int inputChan;

	inputChan = adc_channel(chan);
	if (inputChan < 0)
		xsUnknownError("bad analog input #%d", chan);

	adcSetup();

	ret = read_analog(inputChan);

	adcTerminate();

	xsResult = xsInteger(ret);
}

int adc_channel(uint32_t channel)
{
	switch (channel) {
#ifdef MODDEF_ANALOG_INPUT1
		case 1:
			return MODDEF_ANALOG_INPUT1;
#endif
#ifdef MODDEF_ANALOG_INPUT2
		case 2:
			return MODDEF_ANALOG_INPUT2;
#endif
#ifdef MODDEF_ANALOG_INPUT3
		case 3:
			return MODDEF_ANALOG_INPUT3;
#endif
#ifdef MODDEF_ANALOG_INPUT4
		case 4:
			return MODDEF_ANALOG_INPUT4;
#endif
#ifdef MODDEF_ANALOG_INPUT5
		case 5:
			return MODDEF_ANALOG_INPUT5;
#endif
	}
	return -1;
}

uint32_t read_analog(uint32_t channel)
{
#ifdef MODDEF_ANALOG_REF
	return adcSingle(true, channel, MODDEF_ANALOG_REF);
#else
	return adcSingle(true, channel, adcRefVDD);
#endif
}

