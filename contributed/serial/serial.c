/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#ifdef gecko
	#include "xsPlatform.h"
	#include "xsgecko.h"
#else
	#include "xsesp.h"
#endif
#include "mc.xs.h"			// for xsID_ values

#include "modSerial.h"

/*
	Serial
*/

void xs_serial(xsMachine *the)
{
	modSerialConfiguration serial;
	int baudrate, rx, tx = 0;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_baudrate);
	baudrate = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_rx);
	rx = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_tx);
	tx = xsmcToInteger(xsVar(0));

	serial = xsmcSetHostChunk(xsThis, NULL, sizeof(modSerialConfigurationRecord));
	serial->baudrate = baudrate;
	serial->rx = rx;
	serial->tx = tx;
	modSerialInit(serial);
}

void xs_serial_destructor(void *data)
{
	if (data)
		modSerialUninit((modSerialConfiguration)data);
}

void xs_serial_close(xsMachine *the)
{
	modSerialConfiguration serial = xsmcGetHostChunk(xsThis);
	if (serial) {
		modSerialUninit(serial);
		xsmcSetHostData(xsThis, NULL);		// this clears the host chunk allocated
	}
}

void xs_serial_read(xsMachine *the)
{
	modSerialConfiguration serial;
	int argc = xsmcArgc;
	unsigned int len = xsmcToInteger(xsArg(0)), i;
	unsigned char err;
	unsigned char buffer[40];

	if (len > sizeof(buffer))
		xsUnknownError("40 byte read limit");

	serial = xsmcGetHostChunk(xsThis);
	err = modSerialRead(serial, buffer, len);
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

void xs_serial_write(xsMachine *the)
{
	modSerialConfiguration serial;
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

		{	// assume some kind of array (Array, Uint8Array, etc) (@@ use .buffer if present)
			int l;
			uint8_t j;

			xsmcGet(xsVar(0), xsArg(i), xsID_length);
			l = xsmcToInteger(xsVar(0));
			if ((len + l) > sizeof(buffer))
				xsUnknownError("40 byte write limit");
			for (j = 0; j < l; j++) {
				xsmcGet(xsVar(0), xsArg(i), j);
				buffer[len++] = xsmcToInteger(xsVar(0));
			}
		}
	}

	serial = xsmcGetHostChunk(xsThis);
	err = modSerialWrite(serial, buffer);
	if (err)
		xsUnknownError("write failed");
}

