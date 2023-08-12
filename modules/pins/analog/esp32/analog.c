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

#include "xs.h"
#include "xsHost.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#if kCPUESP32C6 || kCPUESP32C3 || kCPUESP32S3 || kCPUESP32
	#define ADC_RESOLUTION	ADC_BITWIDTH_12
#elif kCPUESP32S2
	#define ADC_RESOLUTION	ADC_BITWIDTH_13
#else
	#error unknown CPU type
#endif
	
void xs_analog_read(xsMachine *the)
{
	int channel = xsToInteger(xsArg(0));
	if (channel < 0 || channel >= SOC_ADC_MAX_CHANNEL_NUM)
		xsRangeError("invalid analog channel number");

	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_unit_init_cfg_t adc_unit_config = {
		.unit_id = ADC_UNIT_2,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	if (ESP_OK != adc_oneshot_new_unit(&adc_unit_config, &adc_handle))	
		xsUnknownError("adc_oneshot_new_unit failed");

	adc_oneshot_chan_cfg_t adc_config = {
		.bitwidth = ADC_RESOLUTION,
		.atten = ADC_ATTEN_DB_11,
	};
	
	if (ESP_OK != adc_oneshot_config_channel(adc_handle, channel, &adc_config)) {
		xsLog("adc_oneshot_config_channel failed");
		goto bail;
	}

	int reading;
	int millivolts;
	if (ESP_OK != adc_oneshot_read(adc_handle, channel, &reading)) {
		xsLog("adc_oneshot_read failed");
		goto bail;
	}

	adc_cali_handle_t cali_handle;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	adc_cali_curve_fitting_config_t cali_config = {
		.unit_id = ADC_UNIT_2,
		.bitwidth = ADC_RESOLUTION,
		.atten = ADC_ATTEN_DB_11,
	};
	if (ESP_OK != adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle)) {
		xsLog("cali_create_scheme_curve_fitting failed");
		goto bail2;
	}
#else
	adc_cali_line_fitting_config_t cali_config = {
		.unit_id = ADC_UNIT_2,
		.bitwidth = ADC_RESOLUTION,
		.atten = ADC_ATTEN_DB_11,
	};
	if (ESP_OK != adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle)) {
		xsLog("cali_create_scheme_line_fitting failed");
		goto bail2;
	}
#endif

	if (ESP_OK != adc_cali_raw_to_voltage(cali_handle, reading, &millivolts)) {
		xsLog("cali_raw_to_voltage failed");
		goto bail2;
	}

	double max = (double)((1 << ADC_RESOLUTION) - 1);
	uint32_t linear_value = c_round(millivolts / 3300.0 * max);

	xsResult = xsInteger(linear_value);

bail2:
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	adc_cali_delete_scheme_curve_fitting(cali_handle);
#else
	adc_cali_delete_scheme_line_fitting(cali_handle);
#endif

bail:
	adc_oneshot_del_unit(adc_handle);
}

