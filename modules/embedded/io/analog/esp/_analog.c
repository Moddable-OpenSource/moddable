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
	Analog - uing ESP8266
*/

#include "user_interface.h"	// esp8266 functions

#include "xsmc.h"			// xs bindings for microcontroller
#ifdef __ets__
	#include "xsHost.h"		// esp platform support
#else
	#error - unsupported platform
#endif
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

struct AnalogRecord {
	xsSlot		obj;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

static uint8_t gInUse;

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;

	if (gInUse)
		xsUnknownError("in use");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);

	gInUse = 1;
}

void xs_analog_destructor_(void *data)
{
	if (data)
		c_free(data);

	gInUse = 0;
}

void xs_analog_close_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	int value = system_adc_read();
	xsmcSetInteger(xsResult, (value > 1023) ? 1023 : value);		// pin, as API returns 1024.
}

/*
	Normatively define values returned so that they are consistent across microcontrollers from various vendors
*/
