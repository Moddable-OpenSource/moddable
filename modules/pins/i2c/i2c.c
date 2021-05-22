/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "modI2C.h"

struct modI2CRecord {
	modI2CConfigurationRecord	state;
	uint8_t						throw;
};

typedef struct modI2CRecord modI2CRecord;
typedef struct modI2CRecord *modI2C;


/*
	I2C
*/

void xs_i2c(xsMachine *the)
{
	modI2C i2c;
	int sda, scl, address, hz = 0, throw = 1, timeout = 0;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_sda);
	sda = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_scl);
	scl = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = xsmcToInteger(xsVar(0));
	if ((address < 0) || (address > 127))
		xsUnknownError("invalid address");

	if (xsmcHas(xsArg(0), xsID_hz)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hz);
		hz = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_throw)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_throw);
		throw = xsmcTest(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0));
	}

	i2c = xsmcSetHostChunk(xsThis, NULL, sizeof(modI2CRecord));

	modI2CConfig(i2c->state, hz, sda, scl, address, timeout);
	i2c->throw = throw;
	modI2CInit(&i2c->state);
}

void xs_i2c_destructor(void *data)
{
	if (data)
		modI2CUninit(&((modI2C)data)->state);
}

void xs_i2c_close(xsMachine *the)
{
	modI2C i2c = xsmcGetHostChunk(xsThis);
	if (i2c) {
		modI2CUninit(&i2c->state);
		xsmcSetHostData(xsThis, NULL);		// this clears the host chunk allocated
	}
}

void xs_i2c_read(xsMachine *the)
{
	modI2C i2c;
	int argc = xsmcArgc;
	unsigned int len = xsmcToInteger(xsArg(0)), i;
	unsigned char err;
	unsigned char buffer[40];

	if (len > sizeof(buffer))
		xsUnknownError("40 byte read limit");

	i2c = xsmcGetHostChunk(xsThis);
	err = modI2CRead(&i2c->state, buffer, len, true);
	if (err)
		return;		// undefined returned on read failure

	if (argc >= 2) {
		int bufferByteLength;
		xsResult = xsArg(1);
		bufferByteLength = xsmcGetArrayBufferLength(xsResult);
		if (bufferByteLength < len)
			xsUnknownError("buffer too small");
		c_memmove(xsmcToArrayBuffer(xsResult), buffer, len);
	}
	else {
		xsmcSetArrayBuffer(xsResult, buffer, len);
		xsResult = xsNew1(xsGlobal, xsID_Uint8Array, xsResult);
	}
}

void xs_i2c_write(xsMachine *the)
{
	modI2C i2c;
	uint8_t argc = xsmcArgc, i;
	unsigned char err;
	uint8_t stop = true;
	unsigned int len = 0;
	unsigned char buffer[40];

	xsmcVars(1);

	for (i = 0; i < argc; i++) {
		xsType t = xsmcTypeOf(xsArg(i));
		if ((xsBooleanType == t) && ((argc - 1) == i)) {
			stop = xsmcToBoolean(xsArg(i));
			continue;
		}
		if ((xsNumberType == t) || (xsIntegerType == t)) {
			if ((len + 1) > sizeof(buffer))
				xsUnknownError("40 byte write limit");
			buffer[len++] = (unsigned char)xsmcToInteger(xsArg(i));
			continue;
		}
		if (xsStringType == t) {
			char *s = xsmcToString(xsArg(i));
			int l = c_strlen(s);
			if ((len + l) > sizeof(buffer))
				xsUnknownError("40 byte write limit");
			c_memcpy(buffer + len, s, l);
			len += l;
			continue;
		}
		if (xsmcIsInstanceOf(xsArg(i), xsArrayBufferPrototype)) {
			int l = xsmcGetArrayBufferLength(xsArg(i));
			if ((len + l) > sizeof(buffer))
				xsUnknownError("40 byte write limit");
			c_memmove(buffer + len, xsmcToArrayBuffer(xsArg(i)), l);
			len += l;
			continue;
		}

		{	// assume some kind of array (Array, Uint8Array, etc) (@@ use .buffer if present)
			int l;
			uint8_t j;

			xsmcGet(xsVar(0), xsArg(i), xsID_length);
			l = xsmcToInteger(xsVar(0));
			if ((len + l) > sizeof(buffer))
				xsUnknownError("40 byte write limit");
			for (j = 0; j < l; j++) {
				xsmcGetIndex(xsVar(0), xsArg(i), j);
				buffer[len++] = xsmcToInteger(xsVar(0));
			}
		}
	}

	i2c = xsmcGetHostChunk(xsThis);
	err = modI2CWrite(&i2c->state, buffer, len, stop);
	if (err) {
		if (i2c->throw)
			xsUnknownError("write failed");
		xsResult = xsNew0(xsGlobal, xsID_Error);
	}
}
