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
	Analog -
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "builtinCommon.h"

struct AnalogRecord {
	xsSlot		obj;
	uint32_t	pin;
	uint8_t 	port;
	uint8_t		channel;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	int8_t i;
	uint32_t pin;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

	if ((pin < 26) || (pin > 28))
		xsRangeError("not analog pin");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->pin = pin;

	adc_init();
	adc_gpio_init(pin);
	adc_select_input(pin - 26);

    builtinUsePin(pin);
}

void xs_analog_destructor_(void *data)
{
	Analog analog = data;
	if (!analog)
		return;

    builtinFreePin(analog->pin);

	c_free(analog);
}

void xs_analog_close_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);;
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);

	xsmcSetInteger(xsResult, adc_read());
}

void xs_analog_get_resolution_(xsMachine *the)
{
	xsmcSetInteger(xsResult, 12);
}
