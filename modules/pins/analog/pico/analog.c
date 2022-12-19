/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

#include "hardware/gpio.h"
#include "hardware/adc.h"

/*
	Analog
*/

void xs_analog_read(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	if (pin < 26 || pin > 29)
		xsRangeError("");

	adc_init();
	adc_gpio_init(pin);
	adc_select_input(pin-26);	// ADC inputs 0-3 (GPIO 26-29)
	int value = adc_read() / 4;		// 0 to 1023
	xsResult = xsInteger(value);
}

