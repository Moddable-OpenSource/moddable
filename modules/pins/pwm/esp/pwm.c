/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "Arduino.h"

void xs_pwm_destructor(void *data)
{
	uintptr_t gpio = (uintptr_t)data;

	if (gpio == (uintptr_t)-1)
		return;

	analogWrite(gpio, 0);
}

void xs_pwm(xsMachine *the)
{
	int gpio;

	xsmcSetHostData(xsThis, (void *)(uintptr_t)-1);

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	gpio = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_port))
		xsUnknownError("no port - esp");

	xsmcSetHostData(xsThis, (void *)(uintptr_t)gpio);
}

void xs_pwm_close(xsMachine *the)
{
	xs_pwm_destructor(xsmcGetHostData(xsThis));
	xsmcSetHostData(xsThis, (void *)(uintptr_t)-1);
}

void xs_pwm_write(xsMachine *the)
{
	uintptr_t gpio = (uintptr_t)xsmcGetHostData(xsThis);
	int value = xsmcToInteger(xsArg(0));	// 0 to 1023... enforced by modulo in analogWrite implementation

	analogWrite(gpio, value);
}

