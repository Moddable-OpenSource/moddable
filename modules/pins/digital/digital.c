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
#if gecko
	#include "xsPlatform.h"
	#include "xsgecko.h"
#else
	#include "xsesp.h"
	#include "Arduino.h"
#endif
#include "mc.xs.h"			// for xsID_ values

#include "modGPIO.h"

/*
	Digital
*/

void xs_digital_configure(xsMachine *the)
{
}

void xs_digital_read(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	uint8_t value;
	modGPIOConfigurationRecord config;

	if (modGPIOInit(&config, NULL, pin, kModGPIOInput))
		xsUnknownError("can't init pin");

	value = modGPIORead(&config);
	modGPIOUninit(&config);

	if (255 == value)
		xsUnknownError("can't read pin");

	xsResult = xsInteger(value);
}

void xs_digital_write(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	int value = xsToInteger(xsArg(1));
	modGPIOConfigurationRecord config;

	if (modGPIOInit(&config, NULL, pin, kModGPIOOutput))
		xsUnknownError("can't init pin");

	modGPIOWrite(&config, value ? 1 : 0);
	modGPIOUninit(&config);
}
