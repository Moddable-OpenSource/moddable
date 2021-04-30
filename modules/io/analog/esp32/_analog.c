/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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
	Analog - uing ESP-IDF
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/adc.h"
#include "esp_adc_cal/include/esp_adc_cal.h"

#define V_REF 1100

struct AnalogRecord {
	xsSlot		obj;
	uint32_t	pin;
	uint8_t		channel;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

#if kCPUESP32S2
	#define ADC_RESOLUTION (13)
	#define ADC_WIDTH ADC_WIDTH_BIT_13
	#define ADC_ATTEN ADC_ATTEN_DB_11

	static const uint8_t gADCMap[ADC1_CHANNEL_MAX] = {		// ADC1 channel to GPIO
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		9,
		10
	};
#else
	#define ADC_RESOLUTION (10)
	#define ADC_WIDTH ADC_WIDTH_BIT_10
	#define ADC_ATTEN ADC_ATTEN_DB_11

	static const uint8_t gADCMap[ADC1_CHANNEL_MAX] = {		// ADC1 channel to GPIO
		36,
		37,
		38,
		39,
		32,
		33,
		34,
		35
	};
#endif

static esp_adc_cal_characteristics_t gCharacteristics;

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	int8_t i, channel = -1;
	uint32_t pin;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

	for (i = 0; i < ADC1_CHANNEL_MAX; i++) {
		if (gADCMap[i] == pin) {
			channel = i;
			break;
		}
	}

	if (channel < 0)
		xsRangeError("not analog pin");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (!gCharacteristics.vref) {
		adc1_config_width(ADC_WIDTH);
		adc1_config_channel_atten(channel, ADC_ATTEN);
		esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, V_REF, &gCharacteristics);
	}

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->pin = pin;
	analog->channel = channel;

    builtinUsePin(pin);
}

void xs_analog_destructor_(void *data)
{
	Analog analog = data;
	if (!analog)
		return;

    builtinFreePin(analog->pin);

	c_free(analog);
}

void xs_analog_close_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);
	if (!analog) return;

	xsForget(analog->obj);
	xs_analog_destructor_(analog);
	xsmcSetHostData(xsThis, NULL);
}

void xs_analog_read_(xsMachine *the)
{
	uint32_t millivolts;
	Analog analog = xsmcGetHostData(xsThis);
	if (!analog)
		xsUnknownError("closed");

	millivolts = esp_adc_cal_raw_to_voltage(adc1_get_raw(analog->channel), &gCharacteristics);

	xsmcSetInteger(xsResult, (millivolts * ((1 << ADC_RESOLUTION) - 1)) / 3300);
}

void xs_analog_get_resolution_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);
	if (!analog)
		xsUnknownError("closed");

	xsmcSetInteger(xsResult, ADC_RESOLUTION);
}
