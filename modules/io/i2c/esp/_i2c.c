/*
 * Copyright (c) 2019-2020 Moddable Tech, Inc.
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
	I2C - uing Arduino twi API

	to do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"		// esp platform support

#ifdef __ets__
	#include "twi.h"			// i2c
#endif

#include "builtinCommon.h"

struct I2CRecord {
	uint32_t					hz;
	uint8_t						data;
	uint8_t						clock;
	uint8_t						address;		// 7-bit
	xsSlot						obj;
	struct I2CRecord			*next;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C;
static I2C gI2CActive;

static void i2cActivate(I2C i2c);
static uint8_t usingPins(uint8_t data, uint8_t clock);

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, address;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_data))
		xsRangeError("data required");
	if (!xsmcHas(xsArg(0), xsID_clock))
		xsRangeError("address required");
	if (!xsmcHas(xsArg(0), xsID_address))
		xsRangeError("address required");

	xsmcGet(xsVar(0), xsArg(0), xsID_data);
	data = builtinGetPin(the, &xsVar(0));
	if ((data < 0) || (data > 16))
		xsRangeError("invalid data");

	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	clock = builtinGetPin(the, &xsVar(0));
	if ((clock < 0) || (clock > 16))
		xsRangeError("invalid clock");

	if (usingPins((uint8_t)data, (uint8_t)clock))
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

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->clock = (uint8_t)clock;
	i2c->data = (uint8_t)data;
	i2c->hz = hz;
	i2c->address = address;

	i2c->next = gI2C;
	gI2C = i2c;

	builtinUsePin(data);
	builtinUsePin(clock);
}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c == gI2CActive)
		gI2CActive = NULL;

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
		builtinFreePin(i2c->data);
		builtinFreePin(i2c->clock);
	}

	c_free(i2c);

	if (NULL == gI2C)
		twi_stop();
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
	int type;
	int err;
	uint8_t stop = true;
	void *buffer;

	if ((xsmcArgc > 1) && !xsmcTest(xsArg(1)))
		stop = false;

	type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
 		length = xsmcToInteger(xsArg(0));
		xsmcSetArrayBuffer(xsResult, NULL, length);
		xsArg(0) = xsResult;
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, &buffer, &length);
		xsmcSetInteger(xsResult, length);
	}

	i2cActivate(i2c);
	err = twi_readFrom(i2c->address, buffer, length, stop);
	if (length == 0) {
		if (err)
			xsmcSetInteger(xsResult, 1);
	}
	else if (err)
		xsUnknownError("i2c read failed");
}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	void *buffer;

	if ((xsmcArgc > 1) && !xsmcTest(xsArg(1)))
		stop = false;

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	i2cActivate(i2c);
	err = twi_writeTo(i2c->address, buffer, length, stop);
	if (length == 0) {
		if (err)
			xsmcSetInteger(xsResult, 1);
	}
	else if (err)
		xsUnknownError("i2c write failed");
}

void i2cActivate(I2C i2c)
{
	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->data == i2c->data) && (gI2CActive->clock == i2c->clock) && (gI2CActive->hz == i2c->hz)))
		return;

	twi_init(i2c->data, i2c->clock);
	twi_setClock(i2c->hz);

	gI2CActive = i2c;
}

uint8_t usingPins(uint8_t data, uint8_t clock)
{
	I2C walker;

	for (walker = gI2C; walker; walker = walker->next) {
		if ((walker->data == data) && (walker->clock == clock))
			return 1;
	}

	return 0;
}
