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

#include "xsmc.h"
#include "xsPlatform.h"
#include "mc.xs.h"

#include "driver/adc.h"
#include "esp_adc_cal/include/esp_adc_cal.h"

#define V_REF 1100

#if kCPUESP32C3
	#define ADC_WIDTH ADC_WIDTH_BIT_12
	#define ADC_ATTEN ADC_ATTEN_DB_11
#elif kCPUESP32S2
	#define ADC_WIDTH ADC_WIDTH_BIT_13
	#define ADC_ATTEN ADC_ATTEN_DB_11
#else
	#define ADC_WIDTH ADC_WIDTH_BIT_10
	#define ADC_ATTEN ADC_ATTEN_DB_11
#endif

typedef struct modAnalogConfigurationRecord modAnalogConfigurationRecord;
typedef struct modAnalogConfigurationRecord *modAnalogConfiguration;

struct modAnalogConfigurationRecord {
	uint8_t channel;
};

void xs_analog(xsMachine *the)
{
	modAnalogConfiguration analog;
	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	int pin = xsmcToInteger(xsVar(0));

	if (pin < 0 || pin >= ADC1_CHANNEL_MAX)
		xsRangeError("invalid analog channel number");

	if (ESP_OK != adc1_config_width(ADC_WIDTH))
		xsUnknownError("can't configure precision for ADC1 peripheral");

	if (ESP_OK != adc1_config_channel_atten(pin, ADC_ATTEN))
		xsUnknownError("can't configure attenuation for requested channel on ADC1 peripheral");

	analog = c_malloc(sizeof(modAnalogConfigurationRecord));
	if (NULL == analog)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, analog);

	analog->channel = pin;
}

void xs_analog_destructor(void *data)
{
	modAnalogConfiguration analog = data;
	if (analog) {
		c_free(analog);
	}
}

void xs_analog_close(xsMachine *the)
{
	modAnalogConfiguration analog = (modAnalogConfiguration)xsmcGetHostData(xsThis);
	if (!analog) return;

	xs_analog_destructor(NULL);
	xsmcSetHostData(xsThis, NULL);
}

static uint32_t analog_read(int channel)
{
	esp_adc_cal_characteristics_t characteristics;
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, V_REF, &characteristics);

	uint32_t reading = adc1_get_raw(channel);
	uint32_t millivolts = esp_adc_cal_raw_to_voltage(reading, &characteristics);
	uint32_t linear_value = c_round(millivolts / 3300.0 * 1023.0);

	return linear_value;
}

void xs_analog_read(xsMachine *the)
{
	modAnalogConfiguration analog = xsmcGetHostData(xsThis);

	if (!analog)
		xsUnknownError("analog uninitialized");

	xsResult = xsInteger(analog_read(analog->channel));
}

void xs_analog_static_read(xsMachine *the)
{
	int channel = xsmcToInteger(xsArg(0));
	if (channel < 0 || channel >= ADC1_CHANNEL_MAX)
		xsRangeError("invalid analog channel number");

	if (ESP_OK != adc1_config_width(ADC_WIDTH))
		xsUnknownError("can't configure precision for ADC1 peripheral");

	if (ESP_OK != adc1_config_channel_atten(channel, ADC_ATTEN))
		xsUnknownError("can't configure attenuation for requested channel on ADC1 peripheral");


	xsResult = xsInteger(analog_read(channel));
}

