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

#include "xs.h"
#include "mc.xs.h"			// for xsID_ values

typedef struct {
	int			pin;
} modServoRecord, *modServo;

void xs_servo_destructor(void *data)
{
}

void xs_servo(xsMachine *the)
{
	modServoRecord ms;

	xsVars(1);
	xsVar(0) = xsGet(xsArg(0), xsID_pin);
	ms.pin = xsToInteger(xsVar(0));

	xsSetHostChunk(xsThis, &ms, sizeof(modServoRecord));
}

void xs_servo_close(xsMachine *the)
{
}

void xs_servo_write(xsMachine *the)
{
	modServo ms = (modServo)xsGetHostChunk(xsThis);
	double degrees = xsToNumber(xsArg(0));

	xsLog("set pin %d to %g degrees\n", ms->pin, degrees);
}

void xs_servo_writeMicroseconds(xsMachine *the)
{
	modServo ms = (modServo)xsGetHostChunk(xsThis);
	int us = xsToInteger(xsArg(0));

	xsLog("set pin %d to %d microseconds\n", ms->pin, us);
}
