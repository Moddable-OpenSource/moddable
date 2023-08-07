/*
 * Copyright (c) 2019-20  Moddable Tech, Inc.
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

#include "nrf_delay.h"
#include "nrf_drv_lpcomp.h"
#include "nrfx_saadc.h"

#define kAnalogResolution 10	// 10 bits
#define kResetReasonLPCOMP (1L << 17)

enum {
	kAnalogWakeCrossingUnknown = -1,
	kAnalogWakeCrossingUp,
	kAnalogWakeCrossingDown,
	kAnalogWakeCrossingUpDown
};

typedef struct modAnalogConfigurationRecord modAnalogConfigurationRecord;
typedef struct modAnalogConfigurationRecord *modAnalogConfiguration;

struct modAnalogConfigurationRecord {
	xsSlot obj;
	xsSlot onWake;
	uint8_t channel;
	uint8_t hasOnWake;
};

static void lpcomp_event_handler(nrf_lpcomp_event_t event);
static void wakeableAnalogDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

uint8_t gAnalogInited = 0;

void xs_analog(xsMachine *the)
{
	modAnalogConfiguration analog;
	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	analog = c_malloc(sizeof(modAnalogConfigurationRecord));
	if (NULL == analog)
		xsUnknownError("out of memory");
		
	xsmcSetHostData(xsThis, analog);

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	analog->channel = xsmcToInteger(xsVar(0));

	analog->hasOnWake = false;
	if (xsmcHas(xsArg(0), xsID_onWake)) {
		xsmcGet(analog->onWake, xsArg(0), xsID_onWake);
		xsRemember(analog->onWake);
		analog->hasOnWake = true;
	}

	if (analog->hasOnWake) {
		nrf_drv_lpcomp_config_t config;
		uint16_t reference, detection;
		int wakeValue, wakeCrossing;
		uint32_t resetReason;
		double scaledValue;
		ret_code_t err_code;

		analog->obj = xsThis;
		if (xsmcHas(xsArg(0), xsID_target)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_target);
			xsmcSet(xsThis, xsID_target, xsVar(0));
		}

		// Check for wake from System OFF sleep
		resetReason = nrf52_get_reset_reason();
		if (kResetReasonLPCOMP == resetReason) {
			modMessagePostToMachine(the, NULL, 0, wakeableAnalogDeliver, analog);
		}

		// Check for wake from System ON sleep
		else {
			uint32_t wakeupMagic = ((uint32_t *)MOD_WAKEUP_REASON_MEM)[0];
			if (MOD_ANALOG_WAKE_MAGIC == wakeupMagic) {
				((uint32_t *)MOD_WAKEUP_REASON_MEM)[0] = 0;
				modMessagePostToMachine(the, NULL, 0, wakeableAnalogDeliver, analog);
			}
		}
		
		if (!xsmcHas(xsArg(0), xsID_wakeCrossing))
			xsUnknownError("wakeCrossing missing");

		if (!xsmcHas(xsArg(0), xsID_wakeValue))
			xsUnknownError("wakeValue missing");
		
		xsmcGet(xsVar(0), xsArg(0), xsID_wakeCrossing);
		wakeCrossing = xsmcToInteger(xsVar(0));
		if (wakeCrossing < kAnalogWakeCrossingUp || wakeCrossing > kAnalogWakeCrossingUpDown)
			xsRangeError("invalid wakeCrossing");
		switch(wakeCrossing) {
			case kAnalogWakeCrossingUp: detection = NRF_LPCOMP_DETECT_UP; break;
			case kAnalogWakeCrossingDown: detection = NRF_LPCOMP_DETECT_DOWN; break;
			case kAnalogWakeCrossingUpDown: detection = NRF_LPCOMP_DETECT_CROSS; break;
		}

		xsmcGet(xsVar(0), xsArg(0), xsID_wakeValue);
		wakeValue = xsmcToInteger(xsVar(0));

		scaledValue = ((double)wakeValue) / (1L << kAnalogResolution);
		reference = (uint16_t)(scaledValue * (LPCOMP_REFSEL_REFSEL_SupplySevenEighthsPrescaling - LPCOMP_REFSEL_REFSEL_SupplyOneEighthPrescaling + 1));

		config.hal.reference = reference;
		config.hal.detection = detection;
		config.hal.hyst = 0;
		config.input = analog->channel;
		config.interrupt_priority = 6;
		err_code = nrf_drv_lpcomp_init(&config, lpcomp_event_handler);
		if (NRF_SUCCESS != err_code)
			xsUnknownError("wakeable analog config failure");

		// Enable the LPCOMP right before System ON (RTC) or System OFF sleep
		//nrf_drv_lpcomp_enable();
	}
}

void xs_analog_destructor(void *data)
{
	modAnalogConfiguration analog = data;
	if (analog) {
		if (analog->hasOnWake)
			nrf_drv_lpcomp_disable();
		c_free(analog);
	}
}

void xs_analog_close(xsMachine *the)
{
	modAnalogConfiguration analog = xsmcGetHostData(xsThis);
	if (!analog) return;

	if (analog->hasOnWake)
		xsForget(analog->onWake);
	xs_analog_destructor(analog);
	xsmcSetHostData(xsThis, NULL);
}

void saadc_handler(nrfx_saadc_evt_t const *event)
{
}

void lpcomp_event_handler(nrf_lpcomp_event_t event)
{
}

void wakeableAnalogDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modAnalogConfiguration analog = refcon;

	xsBeginHost(the);
		xsCallFunction0(analog->onWake, analog->obj);
	xsEndHost(the);
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

nrf_saadc_value_t read_analog(int channel)
{
	nrf_saadc_value_t value;
	ret_code_t err_code;

	init_analog_channel(channel);

	err_code = nrfx_saadc_sample_convert(channel, &value);
	
	return value;
}

void xs_analog_read(xsMachine *the)
{
	modAnalogConfiguration analog = xsmcGetHostData(xsThis);
	int channel = analog->channel;
	nrf_saadc_value_t value;

	if (channel < 0 || channel > (NRF_SAADC_CHANNEL_COUNT -1))
		xsRangeError("invalid analog channel number");
		
	value = read_analog(channel);
	
	xsResult = xsInteger(value);
}

void xs_analog_static_read(xsMachine *the)
{
	int channel = xsmcToInteger(xsArg(0));
	nrf_saadc_value_t value;

	if (channel < 0 || channel > (NRF_SAADC_CHANNEL_COUNT -1))
		xsRangeError("invalid analog channel number");
		
	value = read_analog(channel);
	
	xsResult = xsInteger(value);
}

