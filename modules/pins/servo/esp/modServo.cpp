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
#include "mc.xs.h"			// for xsID_ values

#include "Servo.h"

typedef struct {
	Servo		*s;
	int			min;
	int			max;
} modServoRecord, *modServo;

void xs_servo_destructor(void *data)
{
	if (data) {
		modServo ms = (modServo)data;
		if (ms->s)
			delete ms->s;
	}
}

void xs_servo(xsMachine *the)
{
	modServoRecord ms;
	int pin;

	ms.min = MIN_PULSE_WIDTH;
	ms.max = MAX_PULSE_WIDTH;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_min)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_min);
		ms.min = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_max)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_max);
		ms.max = xsmcToInteger(xsVar(0));
	}

	ms.s = new Servo;
	xsmcSetHostChunk(xsThis, &ms, sizeof(modServoRecord));

	ms.s->attach(pin, ms.min, ms.max);
}

void xs_servo_close(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	if (!ms || !ms->s) return;

	delete ms->s;
	ms->s = NULL;
}

void xs_servo_write(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	double degrees = xsmcToNumber(xsArg(0));
	int us;

	if (!ms || !ms->s) xsUnknownError((char *)"closed");

	us = (((double)(ms->max - ms->min) * degrees) / 180.0) + ms->min;
	ms->s->writeMicroseconds(us);
}

void xs_servo_writeMicroseconds(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	int us = xsmcToInteger(xsArg(0));

	if (!ms || !ms->s) xsUnknownError((char *)"closed");

	ms->s->writeMicroseconds(us);
}

