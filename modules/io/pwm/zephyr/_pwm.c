/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

#if kModZephyrPWMBusCount

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

struct PWMRecord {
	int			hz;
	int			resolution;
	uint8_t		channel;
	const struct device *port;
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

#define RES_BITS_MAX	16

void xs_pwm_constructor_(xsMachine *the)
{
	xsSlot tmp;
	PWM pwm = NULL;
	int hz = 1024 * 3;
	int resolution = 10;
	int channel = 0;

	xsmcVars(2);

	if (!xsmcHas(xsArg(0), xsID_port))
		xsRangeError("port required");
	xsmcGet(tmp, xsArg(0), xsID_port);
	const struct modZephyrPWM *port = modZephyrGetPWM(xsmcToString(tmp));
	if (NULL == port)
		xsRangeError("bad pwm port");

	if (!device_is_ready(port->device))
		xsRangeError("pwm port not ready");

    if (xsmcHas(xsArg(0), xsID_hz)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));
        if (hz <= 0)
			xsRangeError("invalid hz");
    }

    if (xsmcHas(xsArg(0), xsID_channel)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_channel);
        channel = xsmcToInteger(xsVar(0));
        if (channel < 0)
			xsRangeError("invalid channel");
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
		pwm = c_calloc(1, sizeof(PWMRecord));
		if (!pwm)
			xsRangeError("no memory");
	}
	else {
		xsForget(pwm->obj);
		xsmcSetHostData(xsVar(1), NULL);
		xsmcSetHostDestructor(xsVar(1), NULL);
	}

	builtinInitializeTarget(the);

	pwm->hz = hz;
	pwm->resolution = resolution;
	pwm->channel = channel;
	pwm->port = port->device;
	pwm->obj = xsThis;

	xsRemember(pwm->obj);
    xsmcSetHostData(xsThis, pwm);
}

void xs_pwm_destructor_(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

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
	int err;

	int period = PWM_SEC(1) / pwm->hz;
	int value = xsmcToInteger(xsArg(0));

	if ((value < 0) || (value > period))
		xsRangeError("invalid value");

	double cycle = period / (1 << pwm->resolution);
	int pulse = (double)value * cycle;

	err = pwm_set(pwm->port, pwm->channel, period, pulse, 0);
}

void xs_pwm_get_hz_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);
	uint64_t cycles;

	pwm_get_cycles_per_sec(pwm->port, pwm->channel, &cycles);
	xsmcSetInteger(xsResult, cycles);
}

void xs_pwm_get_resolution_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, pwm->resolution);
}

#else // ! kModZephyrPWMBusCount

void xs_pwm_constructor_(xsMachine *the)
{
    xsUnknownError("no PWM");
}

void xs_pwm_destructor_(void *data) {}
void xs_pwm_close_(xsMachine *the) {}
void xs_pwm_write_(xsMachine *the) {}
void xs_pwm_get_hz_(xsMachine *the) {}
void xs_pwm_get_resolution_(xsMachine *the) {}

#endif // kModZephyrPWMBusCount
