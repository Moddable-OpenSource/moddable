/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

#include "modI2C.h"

/*
	I2C
*/

void xs_i2c(xsMachine *the)
{
	modI2CConfiguration i2c;
	int sda, scl, address, hz = 0;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_sda);
	sda = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	scl = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = xsmcToInteger(xsVar(0));
	if ((address < 0) || (address > 127))
		xsUnknownError("invalid address");

	if (xsmcHas(xsArg(0), xsID_hz)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hz);
		hz = xsmcToInteger(xsVar(0));
	}

	i2c = xsmcSetHostChunk(xsThis, NULL, sizeof(modI2CConfigurationRecord));

	i2c->hz = hz;
	i2c->sda = sda;
	i2c->scl = scl;
	i2c->address = address;
	modI2CInit(i2c);
}

void xs_i2c_destructor(void *data)
{
	if (data)
		modI2CUninit((modI2CConfiguration)data);
}

void xs_i2c_close(xsMachine *the)
{
	modI2CConfiguration i2c = xsmcGetHostChunk(xsThis);
	if (i2c) {
		modI2CUninit(i2c);
		xsmcSetHostData(xsThis, NULL);		// this clears the host chunk allocated
	}
}

void xs_i2c_read(xsMachine *the)
{
	modI2CConfiguration i2c;
	int argc = xsmcArgc;
	unsigned int len = xsmcToInteger(xsArg(0)), i;
	unsigned char err;
	unsigned char buffer[34];

	if (len > sizeof(buffer))
		xsUnknownError("34 byte read limit");

	i2c = xsmcGetHostChunk(xsThis);
	err = modI2CRead(i2c, buffer, len, true);
	if (err)
		return;		// undefined returned on read failure

	if (argc >= 2) {
		int bufferByteLength;
		xsResult = xsArg(1);
		bufferByteLength = xsGetArrayBufferLength(xsResult);
		if (bufferByteLength < len)
			xsUnknownError("buffer too small");
		c_memmove(xsmcToArrayBuffer(xsResult), buffer, len);
	}
	else {
		xsResult = xsArrayBuffer(buffer, len);
		xsResult = xsNew1(xsGlobal, xsID_Uint8Array, xsResult);
	}
}

void xs_i2c_write(xsMachine *the)
{
	modI2CConfiguration i2c;
	uint8_t argc = xsmcArgc, i;
	unsigned char err;
	unsigned int len = 0;
	unsigned char buffer[34];

	xsmcVars(1);

	for (i = 0; i < argc; i++) {
		xsType t = xsmcTypeOf(xsArg(i));
		if ((xsNumberType == t) || (xsIntegerType == t)) {
			if ((len + 1) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			buffer[len++] = (unsigned char)xsmcToInteger(xsArg(i));
			continue;
		}
		if (xsStringType == t) {
			char *s = xsmcToString(xsArg(i));
			int l = espStrLen(s);
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			espMemCpy(buffer + len, s, l);
			len += l;
			continue;
		}

		{	// assume some kind of array (Array, Uint8Array, etc) (@@ use .buffer if present)
			int l;
			uint8_t i;

			xsmcGet(xsVar(0), xsArg(i), xsID_length);
			l = xsmcToInteger(xsVar(0));
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			for (i = 0; i < l; i++) {
				xsmcGet(xsVar(0), xsArg(i), i);
				buffer[len++] = xsmcToInteger(xsVar(0));
			}
		}
	}

	i2c = xsmcGetHostChunk(xsThis);
	err = modI2CWrite(i2c, buffer, len, true);
	if (err)
		xsUnknownError("write failed");
}
