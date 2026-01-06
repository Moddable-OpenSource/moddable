/*
 * Copyright (c) 2019-2025 Moddable Tech, Inc.
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
#include "mc.devicetree.h"
#include "xsHost.h"			// platform support

#include "builtinCommon.h"

#include <zephyr/drivers/i2c.h>

#if kModZephyrI2CBusCount

struct I2CRecord {
	uint32_t			hz;
	uint32_t			timeout;
	uint8_t				address;		// 7-bit
	const struct device *port;
	uint32_t			config;
	xsSlot				obj;
	struct I2CRecord	*next;
	const struct device	*inst;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C = NULL;
static I2C gI2CActive = NULL;

static uint8_t i2cActivate(I2C i2c);

static uint32_t hz_bit(uint32_t hz)
{
	if (hz <= 100000)
		return (I2C_SPEED_STANDARD << I2C_SPEED_SHIFT);
	else if (hz <= 400000)
		return (I2C_SPEED_FAST << I2C_SPEED_SHIFT);
	else if (hz <= 1000000)
		return (I2C_SPEED_FAST_PLUS << I2C_SPEED_SHIFT);
	else if (hz <= 3400000)
		return (I2C_SPEED_HIGH << I2C_SPEED_SHIFT);
	else if (hz <= 5000000)
		return (I2C_SPEED_ULTRA << I2C_SPEED_SHIFT);
	else
		return (I2C_SPEED_STANDARD << I2C_SPEED_SHIFT);
}

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int hz, address;
	int timeout = 200;
	xsSlot tmp;
	uint32_t config = 0;

	xsmcVars(2);

	if (!xsmcHas(xsArg(0), xsID_port))
		xsRangeError("port required");
	if (!xsmcHas(xsArg(0), xsID_address))
		xsRangeError("address required");

	xsmcGet(tmp, xsArg(0), xsID_port);
	const struct modZephyrI2C *port = modZephyrGetI2C(xsmcToString(tmp));
	if (NULL == port)
		xsRangeError("bad port");

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = builtinGetPin(the, &xsVar(0));
	if ((address < 0) || (address > 127))
		xsRangeError("invalid address");

	for (i2c = gI2C; i2c; i2c = i2c->next) {
		if ((i2c->address == address) && (i2c->port == port->device))
			xsRangeError("duplicate address");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	config = hz_bit(hz);
	config |= I2C_MODE_CONTROLLER;

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0));
	}

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->hz = hz;
	i2c->address = address;
	i2c->timeout = timeout * 1000;
	i2c->port = port->device;
	i2c->config = config;

	i2c->next = gI2C;
	gI2C = i2c;
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

	err = i2c_read(i2c->port, buffer, length, i2c->address);
//	err = i2c_read_timeout_us(i2c->inst, i2c->address, buffer, length, !stop, i2c->timeout);
	if (err < 0)
		xsUnknownError("read failed");
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

	err = i2c_write(i2c->port, buffer, length, i2c->address);
//	err = i2c_write_timeout_us(i2c->inst, i2c->address, buffer, length, !stop, i2c->timeout);
	if (err < 0)
		xsUnknownError("write failed");
}

void _xs_i2c_writeRead(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	xsUnsignedValue lengthWrite, lengthRead;
	void *bufferWrite, *bufferRead;
	uint8_t stop = true;
	int err;

	if (xsmcArgc > 2)
		stop = xsmcToBoolean(xsArg(2));

	if (xsReferenceType == xsmcTypeOf(xsArg(1))) {
		xsResult = xsArg(1);
		xsmcGetBufferWritable(xsResult, &bufferRead, &lengthRead);
		xsmcSetInteger(xsResult, lengthRead);
	}
	else {
 		lengthRead = xsmcToInteger(xsArg(1));
		bufferRead = xsmcSetArrayBuffer(xsResult, NULL, lengthRead);
	}
	xsmcGetBufferReadable(xsArg(0), &bufferWrite, &lengthWrite);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	err = i2c_write_read(i2c->port, i2c->address, bufferWrite, lengthWrite, bufferRead, lengthRead);
	if (err < 0)
		xsUnknownError("write/read failed");
}

uint8_t i2cActivate(I2C i2c)
{
	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->hz == i2c->hz) && (gI2CActive->port == i2c->port)))
		return 1;

	int err;
	err = i2c_configure(i2c->port, i2c->config);

	gI2CActive = i2c;

	return 1;
}

#else // !kModZephyrI2CBusCount

void _xs_i2c_constructor(xsMachine *the)
{
	xsUnknownError("no I2C");
}

void _xs_i2c_destructor(void *) {}
void _xs_i2c_close(xsMachine *the) {}
void _xs_i2c_read(xsMachine *the) {}
void _xs_i2c_write(xsMachine *the) {}
void _xs_i2c_writeRead(xsMachine *the) {}

#endif
