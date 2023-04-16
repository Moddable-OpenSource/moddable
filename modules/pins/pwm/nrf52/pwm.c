/*
 * Copyright (c) 2018-2023 Moddable Tech, Inc.
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

#include <stdlib.h>
#include "xsmc.h"
#include "mc.xs.h"		// for xsID_ values
#include "mc.defines.h"
#include "xsHost.h"

#include "nrf_drv_pwm.h"

#ifndef MODDEF_PWM_LEDC_FREQUENCY
	#define MODDEF_PWM_LEDC_FREQUENCY 1024
#endif

#define MOD_PWM_PORTS	4

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
	.top_value = MODDEF_PWM_LEDC_FREQUENCY,
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
	.top_value = MODDEF_PWM_LEDC_FREQUENCY,
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
	.top_value = MODDEF_PWM_LEDC_FREQUENCY,
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
	.top_value = MODDEF_PWM_LEDC_FREQUENCY,
	.load_mode = NRF_PWM_LOAD_INDIVIDUAL,
	.step_mode = NRF_PWM_STEP_AUTO
}
};

static nrf_pwm_values_individual_t seq_vals_port0;
static nrf_pwm_values_individual_t seq_vals_port1;
static nrf_pwm_values_individual_t seq_vals_port2;
static nrf_pwm_values_individual_t seq_vals_port3;

static nrf_pwm_sequence_t seq[MOD_PWM_PORTS] =
{
	{
		.values.p_individual = &seq_vals_port0,
		.length = NRF_PWM_VALUES_LENGTH(seq_vals_port0),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &seq_vals_port1,
		.length = NRF_PWM_VALUES_LENGTH(seq_vals_port1),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &seq_vals_port2,
		.length = NRF_PWM_VALUES_LENGTH(seq_vals_port2),
		.repeats = 0,
		.end_delay = 0
	},
	{
		.values.p_individual = &seq_vals_port3,
		.length = NRF_PWM_VALUES_LENGTH(seq_vals_port3),
		.repeats = 0,
		.end_delay = 0
	}
};

struct PWMRecord {
	uint32_t		pin;
	uint8_t			channel;
	uint8_t			port;		// PWM0, PWM1, PWM2, PWM3
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

static uint8_t gPortInitialized = 0;	// (1 << 0), (1 << 1), (1 << 2)
static uint8_t gLEDC[MOD_PWM_PORTS] = { 0xf, 0xf, 0xf, 0xf }; // each port can have 4 gpios
static nrf_drv_pwm_t m_pwm[MOD_PWM_PORTS] = {
	NRF_DRV_PWM_INSTANCE(0),
	NRF_DRV_PWM_INSTANCE(1),
	NRF_DRV_PWM_INSTANCE(2),
	NRF_DRV_PWM_INSTANCE(3)
};

void xs_pwm_destructor(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

	gLEDC[pwm->port] |= 1 << pwm->channel;
	pwmPortConfig[pwm->port].output_pins[pwm->channel] = NRF_DRV_PWM_PIN_NOT_USED;
	if (gLEDC[pwm->port] == 0xf) {
		nrf_drv_pwm_uninit(&m_pwm[pwm->port]);
		gPortInitialized &= ~(1 << pwm->port);
	}

	free(pwm);
}

void xs_pwm(xsMachine *the)
{
	int gpio;
	int port;
	PWM pwm;
	int channel;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	gpio = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
	}
	else
		port = 0;

	if (port > 3)
		xsUnknownError("pwm port out of range");

	for (channel = 0; channel < 4; channel++) {
		if (gLEDC[port] & (1 << channel))
			break;
	}
	if (4 == channel)
		xsUnknownError("no ledc channel free");

	pwmPortConfig[port].output_pins[channel] = gpio;

	if (!(gPortInitialized & (1 << port))) {
		APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm[port], &pwmPortConfig[port], NULL));
		gPortInitialized |= (1 << port);
	}
	else {
		m_pwm[port].p_registers->PSEL.OUT[channel] = gpio;
	}

	pwm = (PWM)malloc(sizeof(PWMRecord));
	if (!pwm)
		xsUnknownError("no memory");

	pwm->channel = channel;
	pwm->port = port;
	pwm->pin = gpio;

	xsmcSetHostData(xsThis, pwm);

	gLEDC[pwm->port] &= ~(1 << channel);
}

void xs_pwm_close(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm) return;
	xs_pwm_destructor(pwm);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pwm_write(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	int value = xsmcToInteger(xsArg(0));	// 0 to 1023
	nrf_pwm_values_individual_t *values;

	if (!pwm) return;

	if ((value < 0) || (value > 1023))
		xsRangeError("bad value");

	if (value == 1023)
		value = 1024;

	switch (pwm->port) {
		case 0: values = &seq_vals_port0; break;
		case 1: values = &seq_vals_port1; break;
		case 2: values = &seq_vals_port2; break;
		case 3: values = &seq_vals_port3; break;
	}

	switch (pwm->channel) {
		case 0: values->channel_0 = value; break;
		case 1: values->channel_1 = value; break;
		case 2: values->channel_2 = value; break;
		case 3: values->channel_3 = value; break;
	}

	nrf_drv_pwm_simple_playback(&m_pwm[pwm->port], &seq[pwm->port], 1, NRF_DRV_PWM_FLAG_LOOP);

}
