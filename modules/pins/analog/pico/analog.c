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

#include "hardware/gpio.h"
#include "hardware/adc.h"

/*
	Analog
*/

typedef struct modAnalogConfigurationRecord modAnalogConfigurationRecord;
typedef struct modAnalogConfigurationRecord *modAnalogConfiguration;

struct modAnalogConfigurationRecord {
	uint8_t		pin;
	uint8_t		channel;
};

void xs_analog(xsMachine *the)
{
	modAnalogConfiguration analog;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	int channel = xsmcToInteger(xsVar(0));

//	if (channel < 26 || channel > 29)
	if (channel < 0 || channel > 2)
		xsRangeError("bad analog channel");

	analog = c_malloc(sizeof(modAnalogConfigurationRecord));
	if (NULL == analog)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, analog);

//	analog->channel = channel - 26;			 // ADC inputs 0-3 (GPIO 26-29)
	analog->channel = channel;
	analog->pin = channel + 26;

	adc_init();
	adc_gpio_init(analog->pin);
}

void xs_analog_destructor(void *data)
{
	modAnalogConfiguration analog = data;
	if (analog) {
		c_free(analog);
	}
}

void xs_analog_close(xsMachine *the)
{
    xs_analog_destructor(NULL);
}

void xs_analog_read(xsMachine *the)
{
	modAnalogConfiguration analog = xsmcGetHostData(xsThis);

	if (!analog)
		xsUnknownError("analog uninitialized");

	adc_select_input(analog->channel + 26);
	int value = adc_read() / 4;		// 0 to 1023
	xsResult = xsInteger(value);
}


void xs_analog_static_read(xsMachine *the)
{
	int pin = xsmcToInteger(xsArg(0));
	if (pin < 0 || pin > 2)
		xsRangeError("bad analog channel");

	adc_init();
	adc_gpio_init(pin + 26);
	adc_select_input(pin);	// ADC inputs 0-3 (GPIO 26-29)

	int value = adc_read() / 4;		// 0 to 1023
	xsResult = xsInteger(value);
}
