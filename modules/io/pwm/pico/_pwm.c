/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
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
	PWM -
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define RES_BITS_MAX	16

struct PWMRecord {
	uint8_t		pin;
	int			hz;
	int			resolution;
	int			duty;
	uint8_t		slice;
	uint8_t		channel;			
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

uint32_t pwm_set_freq_duty(PWM pwm, uint32_t f, int d)
{
	uint32_t clock = clock_get_hz(clk_sys);
	uint32_t clock_div = clock / f / 4096 + (clock % (f * 4096) != 0);
	if (0 == clock_div / 16)
		clock_div = 16;
	uint32_t wrap = clock * 16 / clock_div / f - 1;
	pwm_set_clkdiv_int_frac(pwm->slice, clock_div/16, clock_div & 0xF);
	pwm_set_wrap(pwm->slice, wrap);
	pwm_set_chan_level(pwm->slice, pwm->channel, (wrap * d) / (1 << pwm->resolution));
	return wrap;
}

void xs_pwm_constructor_(xsMachine *the)
{
	PWM pwm = NULL;
	int pin;
	int hz = 1024 * 10;
	int resolution = 10;
	int duty = 0;

	xsmcVars(2);

    if (xsmcHas(xsArg(0), xsID_from)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_from);
		pwm = xsmcGetHostDataValidate(xsVar(1), xs_pwm_destructor_);
		pin = pwm->pin;
		duty = pwm->duty;
		hz = pwm->hz;
		resolution = pwm->resolution;
    }
    else {
		int i = 0;

		xsmcGet(xsVar(0), xsArg(0), xsID_pin);
		pin = builtinGetPin(the, &xsVar(0));

		if (!builtinIsPinFree(pin))
			xsUnknownError("in use");
	}

    if (xsmcHas(xsArg(0), xsID_hz)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));
        if (hz <= 0)
			xsRangeError("invalid hz");
    }

    if (xsmcHas(xsArg(0), xsID_resolution)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_resolution);
        resolution = xsmcToInteger(xsVar(0));
        if ((resolution <= 0) || (resolution >= RES_BITS_MAX))
			xsRangeError("invalid resolution");
    }

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (NULL == pwm) {
		pwm = c_malloc(sizeof(PWMRecord));
		if (!pwm)
			xsRangeError("no memory");
	}
	else {
		xsForget(pwm->obj);
		xsmcSetHostData(xsVar(1), NULL);
		xsmcSetHostDestructor(xsVar(1), NULL);
	}

	builtinInitializeTarget(the);

	pwm->pin = pin;
	pwm->duty = duty;
	pwm->hz = hz;
	pwm->resolution = resolution;
	pwm->obj = xsThis;

	pwm->slice = pwm_gpio_to_slice_num(pin);
	pwm->channel = pwm_gpio_to_channel(pin);

	xsRemember(pwm->obj);
    xsmcSetHostData(xsThis, pwm);

	builtinUsePin(pin);

	gpio_set_function(pin, GPIO_FUNC_PWM);
	pwm_set_freq_duty(pwm, hz, duty);
	pwm_set_enabled(pwm->slice, true);
}

void xs_pwm_destructor_(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

	pwm_set_enabled(pwm->slice, false);
	gpio_deinit(pwm->pin);
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
	int max = (1 << pwm->resolution) - 1;
	int value = xsmcToInteger(xsArg(0));

	if ((value < 0) || (value > max))
		xsRangeError("invalid value");
	if (value == max)
		value += 1;
	pwm->duty = value;

	pwm_set_freq_duty(pwm, pwm->hz, pwm->duty);
}

void xs_pwm_get_hz_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, pwm->hz);
}

void xs_pwm_get_resolution_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, pwm->resolution);
}
