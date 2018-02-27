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
#include "xsesp.h"

#include "driver/adc.h"
#include "esp_adc_cal/include/esp_adc_cal.h"

#define V_REF 1100

void xs_analog_read(xsMachine *the)
{
	int channel = xsToInteger(xsArg(0));
	if (channel < 0 || channel >= ADC1_CHANNEL_MAX)
		xsRangeError("invalid analog channel number");

	if (ESP_OK != adc1_config_width(ADC_WIDTH_BIT_10))
		xsUnknownError("can't configure precision for ADC1 peripheral");

	if (ESP_OK != adc1_config_channel_atten(channel, ADC_ATTEN_DB_11))
		xsUnknownError("can't configure attenuation for requested channel on ADC1 peripheral");

	esp_adc_cal_characteristics_t characteristics;
	esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_10, &characteristics);

	uint32_t milli_volts = adc1_to_voltage(channel, &characteristics);
	uint32_t linear_raw_value = c_round(milli_volts / 3300.0 * 1023.0);

	xsResult = xsInteger(linear_raw_value);
}

