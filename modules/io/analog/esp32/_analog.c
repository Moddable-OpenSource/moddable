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
	Analog - using ESP-IDF
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
	uint8_t 	port;
	uint8_t		channel;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

#if kCPUESP32C3
	#define ADC_RESOLUTION (12)
	#define ADC_WIDTH ADC_WIDTH_BIT_12
	#define ADC_ATTEN ADC_ATTEN_DB_11

	static const uint8_t gADCMap[ADC1_CHANNEL_MAX] = {		// ADC1 channel to GPIO
		0,
		1,
		2,
		3,
		4
	};

	static const uint8_t gADC2Map[ADC2_CHANNEL_MAX] = {		// ADC2 channel to GPIO
		5
	};

#elif kCPUESP32S2 || kCPUESP32S3

	#define ADC_RESOLUTION SOC_ADC_MAX_BITWIDTH		// (13)
	#define ADC_WIDTH SOC_ADC_MAX_BITWIDTH			// ADC_WIDTH_BIT_13
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

	static const uint8_t gADC2Map[ADC2_CHANNEL_MAX] = {		// ADC2 channel to GPIO
		11,
		12,
		13,
		14,
		15,
		16,
		17,
		18,
		19,
		20
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

	static const uint8_t gADC2Map[ADC2_CHANNEL_MAX] = {		// ADC2 channel to GPIO
		4,
		0,
		2,
		15,
		13,
		12,
		14,
		27,
		25,
		26
	};
#endif

static esp_adc_cal_characteristics_t gADC1Characteristics, gADC2Characteristics;

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	int8_t i, channel = -1;
	int8_t port = -1;
	uint32_t pin;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

	for (i = 0; i < ADC1_CHANNEL_MAX; i++) {
		if (gADCMap[i] == pin) {
			channel = i;
			port = 1;
			break;
		}
	}

	if (channel < 0) {
		for (i = 0; i < ADC2_CHANNEL_MAX; i++) {
			if (gADC2Map[i] == pin) {
				channel = i;
				port = 2;
				break;
			}
		}
	}

	if (channel < 0)
		xsRangeError("not analog pin");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (port == 1 && !gADC1Characteristics.vref) {
		adc1_config_width(ADC_WIDTH);
		esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, V_REF, &gADC1Characteristics);
	} else if (port == 2 && !gADC2Characteristics.vref) {
		esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN, ADC_WIDTH, V_REF, &gADC2Characteristics);
	}

	if (port == 1) {
		adc1_config_channel_atten(channel, ADC_ATTEN);
	} else if (port == 2) {
		adc2_config_channel_atten(channel, ADC_ATTEN);
	}

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->pin = pin;
	analog->port = port;
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
	Analog analog = xsmcGetHostData(xsThis);;
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	uint32_t millivolts;
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);

	if (analog->port == 1) {
		millivolts = esp_adc_cal_raw_to_voltage(adc1_get_raw(analog->channel), &gADC1Characteristics);
	} else if (analog->port == 2) {
		int reading;
		if (ESP_OK != adc2_get_raw(analog->channel, ADC_WIDTH, &reading))
			xsUnknownError("ADC2 read timed out"); // can happen routinely if Wi-Fi or BLE is in use

		millivolts = esp_adc_cal_raw_to_voltage(reading, &gADC2Characteristics);
	}

	xsmcSetInteger(xsResult, (millivolts * ((1 << ADC_RESOLUTION) - 1)) / 3300);
}

void xs_analog_get_resolution_(xsMachine *the)
{
	xsmcSetInteger(xsResult, ADC_RESOLUTION);
}
