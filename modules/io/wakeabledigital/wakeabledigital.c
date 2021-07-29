/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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
	WakeableDigital - uing ESP8266 RST

	To do:
		single instance of RST and GPIO wake
		wake from light sleep on GPIO (https://github.com/esp8266/Arduino/issues/3320)
*/

#include "user_interface.h"	// esp8266 functions

#include "xsmc.h"			// xs bindings for microcontroller
#ifdef __ets__
	#include "xsHost.h"		// esp platform support
#else
	#error - unsupported platform
#endif
#include "mc.xs.h"			// for xsID_* values

#define kPinReset (128)

struct WakeableDigitalRecord {
	uint8_t		pin;
	uint8_t		hasOnReadable;
	xsMachine	*the;
	xsSlot		target;
	xsSlot		onReadable;
	struct WakeableDigitalRecord *next;
};
typedef struct WakeableDigitalRecord WakeableDigitalRecord;
typedef struct WakeableDigitalRecord *WakeableDigital;

static void wakeableDigitalDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_wakeabledigital_constructor(xsMachine *the)
{
	WakeableDigital wd;
	int pin, type;
	uint8_t hasOnReadable;
	xsSlot target;

	xsmcVars(1);

	xsmcGet(target, xsArg(0), xsID_target);
	if (!xsmcTest(target))
		target = xsThis;

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);

	type = xsmcTypeOf(xsVar(0));
	if ((xsNumberType == type) || (xsIntegerType == type)) {
		pin = builtinGetPin(the, &xsVar(0));
		if ((pin < 0) || (pin > 15))
			xsRangeError("invalid pin");
		if (!builtinArePinsFree(1 << pin))
			xsUnknownError("in use");
	}
	else if (0 == c_strcmp(xsmcToString(xsVar(0)), "RST"))
		pin = kPinReset;
	else
		xsRangeError("invalid pin");

	hasOnReadable = builtinHasCallback(the, &target, xsID_onReadable);

	wd = c_malloc(hasOnReadable ? sizeof(WakeableDigitalRecord) : offsetof(WakeableDigitalRecord, the));
	if (!wd)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, wd);
	wd->pin = pin;
	wd->hasOnReadable = hasOnReadable;

	if (hasOnReadable) {
		wd->the = the;
		wd->target = target;
		xsRemember(wd->target);

		builtinGetCallback(the, xsID_onReadable, &wd->onReadable);
		xsRemember(wd->onReadable);

		if (kPinReset == wd->pin)
			modMessagePostToMachine(the, NULL, 0, wakeableDigitalDeliver, wd);
	}
}

void xs_wakeabledigital_destructor(void *data)
{
	WakeableDigital wd = data;
	if (!wd) return;

	if (kPinReset != wd->pin)
		builtinFreePins(1 << wd->pin);

	c_free(wd);
}

void xs_wakeabledigital_close(xsMachine *the)
{
	WakeableDigital wd = xsmcGetHostData(xsThis);
	if (!wd) return;

	xsmcSetHostData(xsThis, NULL);
	if (wd->hasOnReadable) {
		xsForget(wd->target);
		xsForget(wd->onReadable);
	}
	xs_wakeabledigital_destructor(wd);
}

void xs_wakeabledigital_read(xsMachine *the)
{
	WakeableDigital wd = xsmcGetHostData(xsThis);

	if (kPinReset == wd->pin)
		xsmcSetInteger(xsResult, (REASON_DEEP_SLEEP_AWAKE == system_get_rst_info()->reason) ? 1 : 0);
	else
		; //@@
}

void wakeableDigitalDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	WakeableDigital wd = refcon;
	xsMachine *the = wd->the;

	xsBeginHost(the);
		xsCallFunction0(wd->onReadable, wd->target);
	xsEndHost(the);
}

