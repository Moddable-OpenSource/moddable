/*
 * Copyright (c) 2019-2023 Moddable Tech, Inc.
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
	I2C
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support

#include "builtinCommon.h"

#include "hardware/i2c.h"

struct I2CRecord {
	uint32_t			hz;
	uint32_t			data;
	uint32_t			clock;
	uint32_t			timeout;
	uint8_t				address;		// 7-bit
	uint8_t				pullup;
	uint8_t				port;
	xsSlot				obj;
	struct I2CRecord	*next;
	void				*inst;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C = NULL;
static I2C gI2CActive = NULL;

static uint8_t i2cActivate(I2C i2c);
static uint8_t usingPins(uint32_t data, uint32_t clock);

uint8_t checkValidI2C(uint32_t data, uint32_t clock, uint8_t *port)
{
	if ((data == 0 || data == 4 || data == 8 || data == 12 || data == 16 || data == 20 || data == 24 || data == 28) &&
		 (clock == 1 || clock == 5 || clock == 9 || clock == 13 || clock == 17 || clock == 21 || clock == 25 || clock == 29)) {
			*port = 0;
			return 1;
	}
	if ((data == 2 || data == 6 || data == 10 || data == 14 || data == 18 || data == 22 || data == 26) &&
		 (clock == 3 || clock == 7 || clock == 11 || clock == 15 || clock == 19 || clock == 23 || clock == 27)) {
			*port = 1;
			return 1;
	}
	return 0;
}

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, address;
	int timeout = 200;
	uint8_t pullup = 1;
	uint8_t port;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_data))
		xsRangeError("data required");
	if (!xsmcHas(xsArg(0), xsID_clock))
		xsRangeError("clock required");
	if (!xsmcHas(xsArg(0), xsID_address))
		xsRangeError("address required");

	xsmcGet(xsVar(0), xsArg(0), xsID_data);
	data = builtinGetPin(the, &xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	clock = builtinGetPin(the, &xsVar(0));

	if (usingPins(data, clock))
		;
	else if (!builtinIsPinFree(data) || !builtinIsPinFree(clock))
		xsRangeError("inUse");

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = builtinGetPin(the, &xsVar(0));
	if ((address < 0) || (address > 127))
		xsRangeError("invalid address");

	for (i2c = gI2C; i2c; i2c = i2c->next) {
		if ((i2c->data == data) && (i2c->clock == clock) && (i2c->address == address))
			xsRangeError("duplicate address");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	if ((hz <= 0) || (hz > 20000000))
		xsRangeError("invalid hz");

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		pullup = xsmcToBoolean(xsVar(0)) ? 1 : 0;
	}

	if (!checkValidI2C(data, clock, &port))
		xsUnknownError("invalid configuration");

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->clock = clock;
	i2c->data = data;
	i2c->hz = hz;
	i2c->address = address;
	i2c->timeout = timeout;
	i2c->pullup = pullup;
	i2c->port = port;
	if (port == 0)
		i2c->inst = i2c0;
	else
		i2c->inst = i2c1;

	i2c->next = gI2C;
	gI2C = i2c;

	builtinUsePin(data);
	builtinUsePin(clock);

	gpio_set_function(data, GPIO_FUNC_I2C);
	gpio_set_function(clock, GPIO_FUNC_I2C);

	if (pullup) {
		gpio_pull_up(data);
		gpio_pull_up(clock);
	}

}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c == gI2CActive) {
		gI2CActive = NULL;
	}

	if (gI2C == i2c)
		gI2C = i2c->next;
	else {
		I2C walker;
		for (walker = gI2C; walker; walker = walker->next) {
			if (walker->next == i2c) {
				walker->next = i2c->next;
				break;
			}
		}
	}

	if (!usingPins(i2c->data, i2c->clock)) {
		if (i2c->pullup) {
			gpio_set_function(i2c->data, GPIO_FUNC_NULL);
			gpio_set_function(i2c->clock, GPIO_FUNC_NULL);
		}
		builtinFreePin(i2c->data);
		builtinFreePin(i2c->clock);
	}

	c_free(i2c);
}

void _xs_i2c_close(xsMachine *the)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (i2c && xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor)) {
		xsForget(i2c->obj);
		_xs_i2c_destructor(i2c);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void _xs_i2c_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	xsUnsignedValue length;
	int err;
	uint8_t stop = true;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, &buffer, &length);
		xsmcSetInteger(xsResult, length);
	}
	else {
 		length = xsmcToInteger(xsArg(0));
		buffer = xsmcSetArrayBuffer(xsResult, NULL, length);
	}

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	err = i2c_read_timeout_us(i2c->inst, i2c->address, buffer, length, !stop, i2c->timeout);
	if (PICO_ERROR_TIMEOUT == err) {
		modLog("i2c_read timeout");
	}
	else if (PICO_ERROR_GENERIC == err) {
		modLog("i2c_read error:");
		modLogInt(err);
		xsUnknownError("read failed");
	}
}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	err = i2c_write_timeout_us(i2c->inst, i2c->address, buffer, length, !stop, i2c->timeout);
	if (PICO_ERROR_TIMEOUT == err) {
		modLog("i2c_write timeout");
	}
	else if (PICO_ERROR_GENERIC == err) {
		modLog("i2c_write error:");
		modLogInt(err);
		xsUnknownError("write failed");
	}
}

uint8_t i2cActivate(I2C i2c)
{
	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->data == i2c->data) && (gI2CActive->clock == i2c->clock) && (gI2CActive->hz == i2c->hz) && (gI2CActive->port == i2c->port) && (gI2CActive->pullup == i2c->pullup)))
		return 1;

	if (gI2CActive) {
		i2c_deinit(gI2CActive->inst);
		gpio_set_function(gI2CActive->data, GPIO_FUNC_NULL);
		gpio_set_function(gI2CActive->clock, GPIO_FUNC_NULL);
		gI2CActive = NULL;
	}

	i2c_init(i2c->inst, i2c->hz);
	gpio_set_function(i2c->data, GPIO_FUNC_I2C);
	gpio_set_function(i2c->clock, GPIO_FUNC_I2C);

	if (i2c->pullup) {
		gpio_pull_up(i2c->data);
		gpio_pull_up(i2c->clock);
	}

	gI2CActive = i2c;

	return 1;
}

uint8_t usingPins(uint32_t data, uint32_t clock)
{
	I2C walker;

	for (walker = gI2C; walker; walker = walker->next) {
		if ((walker->data == data) && (walker->clock == clock))
			return 1;
	}

	return 0;
}
