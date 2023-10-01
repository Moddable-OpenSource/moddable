/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#if kCPUESP32H2 || kCPUESP32C6 || kCPUESP32C3 || kCPUESP32S3 || kCPUESP32
	#define ADC_RESOLUTION	ADC_BITWIDTH_12
#elif kCPUESP32S2
	#define ADC_RESOLUTION	ADC_BITWIDTH_13
#else
	#error unknown CPU type
#endif

#if kCPUESP32C6
	#define ADC1_CHANNEL_MAX	7
	#define ADC2_CHANNEL_MAX	-1
#elif kCPUESP32C3
	#define ADC1_CHANNEL_MAX	4
	#define ADC2_CHANNEL_MAX	0
#elif kCPUESP32S3 || kCPUESP32S2
	#define ADC1_CHANNEL_MAX	9
	#define ADC2_CHANNEL_MAX	9
#elif kCPUESP32
	#define ADC1_CHANNEL_MAX	7
	#define ADC2_CHANNEL_MAX	9
#else
	#error undefined cpu
#endif


#define ADC_ATTEN		ADC_ATTEN_DB_11

typedef struct modAnalogConfigurationRecord modAnalogConfigurationRecord;
typedef struct modAnalogConfigurationRecord *modAnalogConfiguration;

struct modAnalogConfigurationRecord {
	uint8_t						channel;
	adc_oneshot_unit_handle_t	adc_handle;
	adc_cali_handle_t			cali_handle;
};

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	if (!calibrated) {
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_RESOLUTION,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!calibrated) {
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_RESOLUTION,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

	*out_handle = handle;
	if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		modLog("calibration not supported or eFuse not burnt");
	}

	return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	adc_cali_delete_scheme_curve_fitting(handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	adc_cali_delete_scheme_line_fitting(handle);
#endif
}

void xs_analog(xsMachine *the)
{
	modAnalogConfiguration analog;
	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_RESOLUTION,
		.atten = ADC_ATTEN,
	};
	adc_oneshot_unit_init_cfg_t init_config = {
		.unit_id = ADC_UNIT_1,
	};
	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	int pin = xsmcToInteger(xsVar(0));

	if (pin < 0 || pin > ADC1_CHANNEL_MAX)
		xsRangeError("invalid analog channel number");

	if (ESP_OK != adc_oneshot_new_unit(&init_config, &adc_handle))
		xsUnknownError("problems configuring analog unit");

	if (ESP_OK != adc_oneshot_config_channel(adc_handle, pin, &config))
		xsUnknownError("problems configuring analog");

	analog = c_malloc(sizeof(modAnalogConfigurationRecord));
	if (NULL == analog)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, analog);

	analog->channel = pin;
	analog->adc_handle = adc_handle;

	adc_calibration_init(ADC_UNIT_1, ADC_ATTEN, &analog->cali_handle);
}

void xs_analog_destructor(void *data)
{
	modAnalogConfiguration analog = data;
	if (analog) {
		adc_oneshot_del_unit(analog->adc_handle);
		if (analog->cali_handle)
			adc_calibration_deinit(analog->cali_handle);
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

static uint32_t analog_read(modAnalogConfiguration analog)
{
	int raw, voltage = -1;
	if (ESP_OK != adc_oneshot_read(analog->adc_handle, analog->channel, &raw))
		modLog("analog_read failed");

	if (analog->cali_handle) {
		if (ESP_OK != adc_cali_raw_to_voltage(analog->cali_handle, raw, &voltage))
			modLog("analog cali_to_voltage failed");
	}
	if (-1 == voltage)
		voltage = raw;		// uncalibrated

	uint32_t linear_value = c_round(voltage / 3300.0 * 1023.0);

	return linear_value;
}

void xs_analog_read(xsMachine *the)
{
	modAnalogConfiguration analog = xsmcGetHostData(xsThis);

	if (!analog)
		xsUnknownError("analog uninitialized");

	xsResult = xsInteger(analog_read(analog));
}


void xs_analog_static_read(xsMachine *the)
{
	modAnalogConfigurationRecord analog;
	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_RESOLUTION,
		.atten = ADC_ATTEN,
	};
	adc_oneshot_unit_init_cfg_t init_config = {
		.unit_id = ADC_UNIT_1,
	};

	analog.channel = xsmcToInteger(xsArg(0));
	if (analog.channel < 0 || analog.channel > ADC1_CHANNEL_MAX)
		xsRangeError("invalid analog channel number");

	if (ESP_OK != adc_oneshot_new_unit(&init_config, &analog.adc_handle))
		xsUnknownError("problems configuring analog unit");

	if (ESP_OK != adc_oneshot_config_channel(analog.adc_handle, analog.channel, &config))
		xsUnknownError("problems configuring analog");

	adc_calibration_init(ADC_UNIT_1, ADC_ATTEN, &analog.cali_handle);

	xsResult = xsInteger( analog_read(&analog) );

	adc_oneshot_del_unit(analog.adc_handle);
	if (analog.cali_handle)
		adc_calibration_deinit(analog.cali_handle);
}

