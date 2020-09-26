/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

#include "nrfx_saadc.h"

uint8_t gAnalogInited = 0;

void saadc_handler(nrfx_saadc_evt_t const *event)
{
}

void init_analog()
{
	ret_code_t err_code;

	if (!gAnalogInited) {
		nrfx_saadc_config_t config = {
			.resolution = (nrf_saadc_resolution_t)1,	// 10 bit
			.oversample = (nrf_saadc_oversample_t)0,
			.interrupt_priority = 6,
			.low_power_mode = 0
		};

		err_code = nrfx_saadc_init(&config, saadc_handler);
		APP_ERROR_CHECK(err_code);

		gAnalogInited = 1;
	}
}

void init_analog_channel(int channel)
{
	ret_code_t err_code;
	nrf_saadc_input_t nrfAIN;
	
	switch (channel) {
		case 0: nrfAIN = NRF_SAADC_INPUT_AIN0; break;
		case 1: nrfAIN = NRF_SAADC_INPUT_AIN1; break;
		case 2: nrfAIN = NRF_SAADC_INPUT_AIN2; break;
		case 3: nrfAIN = NRF_SAADC_INPUT_AIN3; break;
		case 4: nrfAIN = NRF_SAADC_INPUT_AIN4; break;
		case 5: nrfAIN = NRF_SAADC_INPUT_AIN5; break;
		case 6: nrfAIN = NRF_SAADC_INPUT_AIN6; break;
		default: nrfAIN = NRF_SAADC_INPUT_AIN7; break;
	}

//NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);
	nrf_saadc_channel_config_t config = {
		.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
		.gain 		= NRF_SAADC_GAIN1_6,
		.reference	= NRF_SAADC_REFERENCE_INTERNAL,
		.acq_time	= NRF_SAADC_ACQTIME_10US,
		.mode		= NRF_SAADC_MODE_SINGLE_ENDED,
		.burst		= NRF_SAADC_BURST_DISABLED,
		.pin_p		= nrfAIN,
		.pin_n		= NRF_SAADC_INPUT_DISABLED
	};

	init_analog();

	if (!nrf_saadc_enable_check())
		nrf_saadc_enable();

	err_code = nrfx_saadc_channel_init(channel, &config);
	APP_ERROR_CHECK(err_code);
}

void xs_analog_read(xsMachine *the)
{
	ret_code_t err_code;
	int channel = xsToInteger(xsArg(0));

	if (channel < 0 || channel > (NRF_SAADC_CHANNEL_COUNT -1))
		xsRangeError("invalid analog channel number");

	init_analog_channel(channel);

	nrf_saadc_value_t value;

	err_code = nrfx_saadc_sample_convert(channel, &value);
	
	xsResult = xsInteger(value);
}

