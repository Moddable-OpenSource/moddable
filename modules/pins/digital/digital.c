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

#include "modGPIO.h"

typedef struct modDigitalWakeConfigurationRecord modDigitalWakeConfigurationRecord;
typedef struct modDigitalWakeConfigurationRecord *modDigitalWakeConfiguration;

struct modDigitalWakeConfigurationRecord {
	xsSlot obj;
	xsSlot onWake;
};

void wakeableDigitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

/*
	Digital
*/

void xs_digital_destructor(void *data)
{
	if (data)
		modGPIOUninit((modGPIOConfiguration)data);
}

void xs_digital(xsMachine *the)
{
	int pin, mode, argc = xsmcArgc;
	char *port = NULL;
	modGPIOConfigurationRecord gpio;
	int woke;

	if (1 == argc) {		//@@ eventually only dictionary case should be here
		xsmcVars(1);

		if (!xsmcHas(xsArg(0), xsID_pin))
			xsUnknownError("pin missing");

		if (!xsmcHas(xsArg(0), xsID_mode))
			xsUnknownError("mode missing");

		xsmcGet(xsVar(0), xsArg(0), xsID_pin);
		pin = xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		mode = xsmcToInteger(xsVar(0));

		if (xsmcHas(xsArg(0), xsID_port)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = xsmcToString(xsVar(0));
		}
		
		if (xsmcHas(xsArg(0), xsID_wakeEdge)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_wakeEdge);
			mode |= xsmcToInteger(xsVar(0));
		}
	}
	else if (2 == argc) {
		pin = xsmcToInteger(xsArg(0));
		mode = xsmcToInteger(xsArg(1));
	}
	else if (3 == argc) {
		pin = xsmcToInteger(xsArg(1));
		mode = xsmcToInteger(xsArg(2));
		if (xsmcTest(xsArg(0)))
			port = xsmcToString(xsArg(0));
	}

	woke = modGPIODidWake(NULL, pin);

	if (modGPIOInit(&gpio, port, (uint8_t)pin, mode))
		xsUnknownError("can't init pin");
		
	xsmcSetHostChunk(xsThis, &gpio, sizeof(gpio));
		
	if (xsmcHas(xsArg(0), xsID_onWake)) {
		if (woke) {
			modDigitalWakeConfigurationRecord wake;
			wake.obj = xsThis;
			if (xsmcHas(xsArg(0), xsID_target)) {
				xsmcGet(xsVar(0), xsArg(0), xsID_target);
				xsmcSet(xsThis, xsID_target, xsVar(0));
			}
			xsmcGet(wake.onWake, xsArg(0), xsID_onWake);
			xsRemember(wake.onWake);
			modMessagePostToMachine(the, (uint8_t*)&wake, sizeof(wake), wakeableDigitalDeliver, NULL);
		}
	}
}

void xs_digital_close(xsMachine *the)
{
	xs_digital_destructor(xsmcGetHostChunk(xsThis));
	xsmcSetHostData(xsThis, NULL);
}

void xs_digital_mode(xsMachine *the)
{
	modGPIOConfiguration gpio = xsmcGetHostChunk(xsThis);
	int mode = xsmcToInteger(xsArg(0));

	if (modGPIOSetMode(gpio, mode))
		xsUnknownError("unsupported mode");
}

void xs_digital_read(xsMachine *the)
{
	modGPIOConfiguration gpio = xsmcGetHostChunk(xsThis);
	int value;

	value = modGPIORead(gpio);
	if (kModGPIOReadError == value)
		xsUnknownError("can't read pin");

	xsmcSetInteger(xsResult, value);
}

void xs_digital_write(xsMachine *the)
{
	modGPIOConfiguration gpio = xsmcGetHostChunk(xsThis);
	int value = xsmcToInteger(xsArg(0));
	modGPIOWrite(gpio, value);
}

void xs_digital_static_read(xsMachine *the)
{
	int pin = xsmcToInteger(xsArg(0));
	uint8_t value;
	modGPIOConfigurationRecord config;

	if (modGPIOInit(&config, NULL, pin, kModGPIOInput))
		xsUnknownError("can't init pin");

	value = modGPIORead(&config);
	modGPIOUninit(&config);

	if (kModGPIOReadError == value)
		xsUnknownError("can't read pin");

	xsmcSetInteger(xsResult, value);
}

void xs_digital_static_write(xsMachine *the)
{
	int pin = xsmcToInteger(xsArg(0));
	int value = xsmcToInteger(xsArg(1));
	modGPIOConfigurationRecord config;

	if (modGPIOInit(&config, NULL, pin, kModGPIOOutput))
		xsUnknownError("can't init pin");

	modGPIOWrite(&config, value ? 1 : 0);
	modGPIOUninit(&config);
}

void wakeableDigitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalWakeConfiguration wake = (modDigitalWakeConfiguration)message;

	xsBeginHost(the);
		xsCallFunction0(wake->onWake, wake->obj);
		xsForget(wake->onWake);
	xsEndHost(the);
}
