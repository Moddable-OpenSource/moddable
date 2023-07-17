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

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values
#include "mc.defines.h"

#include "builtinCommon.h"

#include "nrf_drv_pwm.h"

#define NRF52_PWM_RESOLUTION_MAX 15

#define MOD_PWM_PORTS	4

struct PWMPortRecord {
	uint8_t		inUse;
	uint16_t	resolution;
	int			hz;
	int			nrfHz;
};
typedef struct PWMPortRecord PWMPortRecord;
typedef struct PWMPortRecord *PWMPort;

static PWMPortRecord gPWMPorts[MOD_PWM_PORTS];

static uint8_t gPWMChannels[MOD_PWM_PORTS] = { 0xf, 0xf, 0xf, 0xf };

static nrf_drv_pwm_t m_pwm[MOD_PWM_PORTS] = {
	NRF_DRV_PWM_INSTANCE(0),
	NRF_DRV_PWM_INSTANCE(1),
	NRF_DRV_PWM_INSTANCE(2),
	NRF_DRV_PWM_INSTANCE(3)
};

static nrf_drv_pwm_config_t pwmPortConfig[MOD_PWM_PORTS] = {
{
	.output_pins = {
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 0
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 1
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 2
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 3
	},
	.irq_priority = APP_IRQ_PRIORITY_LOWEST,
	.base_clock = NRF_PWM_CLK_125kHz,
	.count_mode = NRF_PWM_MODE_UP,
	.top_value = (1 << NRF52_PWM_RESOLUTION_MAX) - 1,
	.load_mode = NRF_PWM_LOAD_INDIVIDUAL,
	.step_mode = NRF_PWM_STEP_AUTO
},
{
	.output_pins = {
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 0
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 1
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 2
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 3
	},
	.irq_priority = APP_IRQ_PRIORITY_LOWEST,
	.base_clock = NRF_PWM_CLK_125kHz,
	.count_mode = NRF_PWM_MODE_UP,
	.top_value = (1 << NRF52_PWM_RESOLUTION_MAX) - 1,
	.load_mode = NRF_PWM_LOAD_INDIVIDUAL,
	.step_mode = NRF_PWM_STEP_AUTO
},
{
	.output_pins = {
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 0
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 1
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 2
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 3
	},
	.irq_priority = APP_IRQ_PRIORITY_LOWEST,
	.base_clock = NRF_PWM_CLK_125kHz,
	.count_mode = NRF_PWM_MODE_UP,
	.top_value = (1 << NRF52_PWM_RESOLUTION_MAX) - 1,
	.load_mode = NRF_PWM_LOAD_INDIVIDUAL,
	.step_mode = NRF_PWM_STEP_AUTO
},
{
	.output_pins = {
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 0
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 1
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 2
		NRF_DRV_PWM_PIN_NOT_USED,		// channel 3
	},
	.irq_priority = APP_IRQ_PRIORITY_LOWEST,
	.base_clock = NRF_PWM_CLK_125kHz,
	.count_mode = NRF_PWM_MODE_UP,
	.top_value = (1 << NRF52_PWM_RESOLUTION_MAX) - 1,
	.load_mode = NRF_PWM_LOAD_INDIVIDUAL,
	.step_mode = NRF_PWM_STEP_AUTO
}
};

static nrf_pwm_values_individual_t pwm_seq_vals_port0;
static nrf_pwm_values_individual_t pwm_seq_vals_port1;
static nrf_pwm_values_individual_t pwm_seq_vals_port2;
static nrf_pwm_values_individual_t pwm_seq_vals_port3;

static nrf_pwm_sequence_t pwm_seq[MOD_PWM_PORTS] =
{
	{
		.values.p_individual = &pwm_seq_vals_port0,
		.length = NRF_PWM_VALUES_LENGTH(pwm_seq_vals_port0),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &pwm_seq_vals_port1,
		.length = NRF_PWM_VALUES_LENGTH(pwm_seq_vals_port1),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &pwm_seq_vals_port2,
		.length = NRF_PWM_VALUES_LENGTH(pwm_seq_vals_port2),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &pwm_seq_vals_port3,
		.length = NRF_PWM_VALUES_LENGTH(pwm_seq_vals_port3),
		.repeats = 0,
		.end_delay = 0
	}
};

struct PWMRecord {
	uint16_t	pin;
	uint8_t		port;
	uint8_t		channel;
	int			duty;
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

static nrf_pwm_clk_t freq2nrfFreq(int freq)
{
	if (freq <= 131072)
		return NRF_PWM_CLK_125kHz;
	if (freq <= 262144)
		return NRF_PWM_CLK_250kHz;
	if (freq <= 524288)
		return NRF_PWM_CLK_500kHz;
	if (freq <= 1048576)
		return NRF_PWM_CLK_1MHz;
	if (freq <= 2097152)
		return NRF_PWM_CLK_2MHz;
	if (freq <= 4194304)
		return NRF_PWM_CLK_4MHz;
	if (freq <= 8388608)
		return NRF_PWM_CLK_8MHz;

	return NRF_PWM_CLK_16MHz;		// 16777216
}

void xs_pwm_constructor_(xsMachine *the)
{
	PWM pwm = NULL;
	int pin;
	int hz = 125000, nrfHz = -1;
	int resolution = 10;
	int8_t i, port = -1, freePort = -1;
	int channel = -1, duty = 0;

	xsmcVars(2);

    if (xsmcHas(xsArg(0), xsID_from)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_from);
		pwm = xsmcGetHostDataValidate(xsVar(1), xs_pwm_destructor_);
		pin = pwm->pin;
		port = pwm->port;
		channel = pwm->channel;
		duty = pwm->duty;
		hz = gPWMPorts[pwm->port].hz;
		resolution = gPWMPorts[pwm->port].resolution;
    }
    else {
		xsmcGet(xsVar(0), xsArg(0), xsID_pin);
		pin = builtinGetPin(the, &xsVar(0));

		if (!builtinIsPinFree(pin))
			xsUnknownError("in use");

		if (xsmcHas(xsArg(0), xsID_port)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = builtinGetSignedInteger(the, &xsVar(0));
			if ((port < 0) || (port >= MOD_PWM_PORTS))
				xsRangeError("invalid port");
			if (0 == gPWMChannels[port])		// no more channels available
				xsUnknownError("port unavailable");
		}
	}

    if (xsmcHas(xsArg(0), xsID_hz)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));
        if (hz <= 0)
			xsRangeError("invalid hz");
    }

	nrfHz = freq2nrfFreq(hz);
	if (-1 == nrfHz)
		xsUnknownError("bad frequency");

    if (xsmcHas(xsArg(0), xsID_resolution)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_resolution);
        resolution = xsmcToInteger(xsVar(0));
        if ((resolution <= 0) || (resolution > NRF52_PWM_RESOLUTION_MAX))
			xsRangeError("invalid resolution");
    }

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (-1 == port) {
		for (i = 0; i<MOD_PWM_PORTS; i++) {
			if ( (resolution == gPWMPorts[i].resolution)
				&& (freq2nrfFreq(hz) == gPWMPorts[i].nrfHz)
				&& gPWMChannels[i]) {
				port = i;
				break;
			}
			if (0xf == gPWMChannels[i])		// no used channels, free to config
				freePort = i;
		}
	}

	if (-1 == port) 	// no match of hz and resolution
		if (-1 != freePort) 	// set up a new port
			port = freePort;
		else
			xsUnknownError("no free port");

	if (-1 == channel) {
		for (channel = 0; channel < 4; channel++) {
			if (gPWMChannels[port] & (1 << channel))
				break;
		}
		if (4 == channel)
			xsUnknownError("no ledc channel");
	}

	pwmPortConfig[port].output_pins[channel] = pin;
	pwmPortConfig[port].top_value = (1 << resolution) - 1;

	pwmPortConfig[port].base_clock = nrfHz;			// need to map the value the change

	if (gPWMPorts[port].inUse == 0)
        APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm[port], &pwmPortConfig[port], NULL));
	else
		m_pwm[port].p_registers->PSEL.OUT[channel] = pin;

	gPWMPorts[port].inUse++;

	builtinInitializeTarget(the);

	if (NULL == pwm) {
		pwm = c_malloc(sizeof(PWMRecord));
		if (!pwm) {
			//@@ disable ledc_channel
			xsRangeError("no memory");
		}
	}
	else {
		gPWMPorts[pwm->port].inUse -= 1;
		xsForget(pwm->obj);
		xsmcSetHostData(xsVar(1), NULL);
		xsmcSetHostDestructor(xsVar(1), NULL);
	}

	gPWMPorts[port].resolution = resolution;
	gPWMPorts[port].hz = hz;			// need to map the value the change
	gPWMPorts[port].nrfHz = nrfHz;

	pwm->pin = pin;
	pwm->port = port;
	pwm->channel = channel;
	pwm->duty = duty;
	pwm->obj = xsThis;

	xsRemember(pwm->obj);
    xsmcSetHostData(xsThis, pwm);

	gPWMChannels[port] &= ~(1 << channel);

    builtinUsePin(pin);
}

void xs_pwm_destructor_(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

	gPWMChannels[pwm->port] |= 1 << pwm->channel;
	gPWMPorts[pwm->port].inUse -= 1;
	pwmPortConfig[pwm->port].output_pins[pwm->channel] = NRF_DRV_PWM_PIN_NOT_USED;
//	if (gPWMChannels[pwm->port] == 0xf)
	if (gPWMPorts[pwm->port].inUse == 0) {
		nrf_drv_pwm_uninit(&m_pwm[pwm->port]);
	}

    builtinFreePin(pwm->pin);

    c_free(pwm);
}

void xs_pwm_close_(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (pwm && xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_)) {
		xsForget(pwm->obj);
		xs_pwm_destructor_(pwm);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_pwm_write_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);
	int max = (1 << gPWMPorts[pwm->port].resolution) - 1;
	int value = xsmcToInteger(xsArg(0));
	nrf_pwm_values_individual_t *values;

	if ((value < 0) || (value > max))
		xsRangeError("invalid value");

	pwm->duty = value;

	switch (pwm->port) {
		case 0: values = &pwm_seq_vals_port0; break;
		case 1: values = &pwm_seq_vals_port1; break;
		case 2: values = &pwm_seq_vals_port2; break;
		case 3: values = &pwm_seq_vals_port3; break;
	}

	switch (pwm->channel) {
		case 0: values->channel_0 = value; break;
		case 1: values->channel_1 = value; break;
		case 2: values->channel_2 = value; break;
		case 3: values->channel_3 = value; break;
	}

	nrf_drv_pwm_simple_playback(&m_pwm[pwm->port], &pwm_seq[pwm->port], 1, NRF_DRV_PWM_FLAG_LOOP);
}

void xs_pwm_get_hz_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, gPWMPorts[pwm->port].hz);
}

void xs_pwm_get_resolution_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, gPWMPorts[pwm->port].resolution);
}
