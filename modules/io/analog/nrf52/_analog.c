/*
 * Copyright (c) 2019-2023  Moddable Tech, Inc.
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
	Analog
*/

#include "xsmc.h"
#include "xsPlatform.h"
#include "mc.xs.h"
#include "builtinCommon.h"

#include "nrf_delay.h"
#include "nrf_drv_lpcomp.h"
#include "nrfx_saadc.h"

struct AnalogRecord {
	xsSlot	obj;
	uint8_t	channel;
	uint8_t pin;
	uint8_t resolution;
	nrf_saadc_resolution_t nrfRes;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

#define kAnalogResolution 12
#define ADC_CHANNEL_MAX	8

static const uint8_t gADCMap[ADC_CHANNEL_MAX] = {
	2,
	3,
	4,
	5,
	28,
	29,
	30,
	31
};

static uint8_t sAnalogInited = 0;

static nrf_saadc_input_t chan_to_nrfChan(int channel) {
//modLog("channel");
//modLogInt(channel);
	switch (channel) {
		case 0: return NRF_SAADC_INPUT_AIN0;
		case 1: return NRF_SAADC_INPUT_AIN1;
		case 2: return NRF_SAADC_INPUT_AIN2;
		case 3: return NRF_SAADC_INPUT_AIN3;
		case 4: return NRF_SAADC_INPUT_AIN4;
		case 5: return NRF_SAADC_INPUT_AIN5;
		case 6: return NRF_SAADC_INPUT_AIN6;
		case 7: return NRF_SAADC_INPUT_AIN7;
	}
	return NRF_SAADC_INPUT_DISABLED;
}

static void saadc_handler(nrfx_saadc_evt_t const *event)
{
}

static void init_analog(Analog analog)
{
	ret_code_t err_code;

	if (!sAnalogInited) {
		nrfx_saadc_config_t config = {
			.resolution = analog->nrfRes,
			.oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
			.interrupt_priority = 6,
			.low_power_mode = 0
		};

		err_code = nrfx_saadc_init(&config, saadc_handler);
		APP_ERROR_CHECK(err_code);

		sAnalogInited = 1;
	}
}

static void init_analog_channel(Analog analog)
{
	ret_code_t err_code;
	nrf_saadc_input_t nrfAIN = NRF_SAADC_INPUT_DISABLED;

	switch (analog->channel) {
		case 0: nrfAIN = NRF_SAADC_INPUT_AIN0; break;
		case 1: nrfAIN = NRF_SAADC_INPUT_AIN1; break;
		case 2: nrfAIN = NRF_SAADC_INPUT_AIN2; break;
		case 3: nrfAIN = NRF_SAADC_INPUT_AIN3; break;
		case 4: nrfAIN = NRF_SAADC_INPUT_AIN4; break;
		case 5: nrfAIN = NRF_SAADC_INPUT_AIN5; break;
		case 6: nrfAIN = NRF_SAADC_INPUT_AIN6; break;
		case 7: nrfAIN = NRF_SAADC_INPUT_AIN7; break;
	}

	nrf_saadc_channel_config_t config = {
		.resistor_p	= NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n	= NRF_SAADC_RESISTOR_DISABLED,
		.gain		= NRF_SAADC_GAIN1_6,
		.reference	= NRF_SAADC_REFERENCE_INTERNAL,
		.acq_time	= NRF_SAADC_ACQTIME_10US,
		.mode		= NRF_SAADC_MODE_SINGLE_ENDED,
		.burst		= NRF_SAADC_BURST_DISABLED,
		.pin_p		= nrfAIN,
		.pin_n		= NRF_SAADC_INPUT_DISABLED
	};

	if (!nrf_saadc_enable_check())
		nrf_saadc_enable();

	err_code = nrfx_saadc_channel_init(analog->channel, &config);
	APP_ERROR_CHECK(err_code);
}


void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	int8_t i, channel = -1;
	uint32_t pin, resolution;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

	resolution = kAnalogResolution;

	if (!builtinIsPinFree(pin))
		xsRangeError("in use");

	for (i=0; i<ADC_CHANNEL_MAX; i++) {
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

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->pin = pin;
	analog->channel = channel;
	analog->resolution = resolution;

	switch (resolution) {
		case 8: analog->nrfRes = NRF_SAADC_RESOLUTION_8BIT; break;
		case 10: analog->nrfRes = NRF_SAADC_RESOLUTION_10BIT; break;
		case 12: analog->nrfRes = NRF_SAADC_RESOLUTION_12BIT; break;
		case 14: analog->nrfRes = NRF_SAADC_RESOLUTION_14BIT; break;
	}

	builtinUsePin(pin);

	init_analog(analog);
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
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	nrf_saadc_value_t value;
	ret_code_t err_code;
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);

	init_analog_channel(analog);

	err_code = nrfx_saadc_sample_convert(analog->channel, &value);

	xsmcSetInteger(xsResult, value);
}

void xs_analog_get_resolution_(xsMachine *the)
{
	xsmcSetInteger(xsResult, kAnalogResolution);
}

