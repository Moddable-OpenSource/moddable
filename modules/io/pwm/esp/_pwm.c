/*
 * Copyright (c) 2019-2020  Moddable Tech, Inc.
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
	PWM - uing ESP8266 Arduino Core
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"		// esp platform support

#ifdef __ets__
	#include "Arduino.h"
#endif

#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

static uint32_t gFrequencyOwners = 0;
static uint32_t gHz = 0;

struct PWMRecord {
	int			pin;
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

void xs_pwm_constructor_(xsMachine *the)
{
	PWM pwm;
	int pin;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

    if (xsmcHas(xsArg(0), xsID_hz)) {
        int hz;

        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));

        if (gHz != hz) {
            if (gFrequencyOwners)
                xsRangeError("different hz already set by open pwm");

            gHz = hz;
            analogWriteFreq(hz);
        }
        gFrequencyOwners |= (1 << pin);
    }

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");
	builtinInitializeTarget(the);

	pwm = c_malloc(sizeof(PWMRecord));
	if (!pwm)
		xsRangeError("no memory");

	pwm->pin = pin;
	pwm->obj = xsThis;
	xsRemember(pwm->obj);
    xsmcSetHostData(xsThis, pwm);

    analogWrite(pin, 0);
    builtinUsePin(pin);
}

void xs_pwm_destructor_(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

    analogWrite(pwm->pin, 0);
    
    builtinFreePin(pwm->pin);
    gFrequencyOwners &= ~(1 << pwm->pin);

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

   analogWrite(pwm->pin, xsmcToInteger(xsArg(0)));
}

void xs_pwm_get_hz_(xsMachine *the)
{
	extern uint32_t pwm_freq;	// from core_esp8266_wiring_pwm
	xsmcSetInteger(xsResult, pwm_freq);
}
