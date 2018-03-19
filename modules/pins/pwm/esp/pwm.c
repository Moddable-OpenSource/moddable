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
#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

#include "Arduino.h"
#include "modGPIO.h"
#include "modI2C.h"

/*
	PWM
*/

void xs_pwm_write(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	int value = xsToInteger(xsArg(1));	// 0 to 1023... enforced by modulo in analogWrite implementation
	uint8 argc = xsToInteger(xsArgc);
	if (argc > 2){
		int frequency = xsToInteger(xsArg(2));
		analogWriteFreq(frequency);
	}
	analogWrite(pin, value);
}
