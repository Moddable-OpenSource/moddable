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
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

#include "hardware/pwm.h"

struct PWMRecord {
	uint32_t	pin;
	uint8_t		slice;
	pwm_config	config;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

void xs_pwm_destructor(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

	pwm_set_enabled(pwm->slice, false);
	free(pwm);
}

void xs_pwm(xsMachine *the)
{
	int gpio;
	uint32_t slice;
	PWM pwm;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	gpio = xsmcToInteger(xsVar(0));

	pwm = (PWM)malloc(sizeof(PWMRecord));
	if (!pwm)
		xsUnknownError("no memory");

	gpio_set_function(gpio, GPIO_FUNC_PWM);
	slice = pwm_gpio_to_slice_num(gpio);

	pwm->slice = slice;
	pwm->pin = gpio;

	pwm->config = pwm_get_default_config();
	pwm_config_set_clkdiv(&pwm->config, 4.f);
	pwm_config_set_wrap(&pwm->config, 1023);
	pwm_init(slice, &pwm->config, true);

	xsmcSetHostData(xsThis, (void *)pwm);
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
	if (!pwm) return;

	int value = xsmcToInteger(xsArg(0));	// 0 to 1023... enforced by modulo in analogWrite implementation

	pwm_set_gpio_level(pwm->pin, value % 1024);
}

