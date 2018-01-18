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

#include "xsPlatform.h"
#include "xsmc.h"

void xs_hex_toString(xsMachine *the)
{
	int argc = xsmcArgc;
	const uint8_t *bytes;
	char *string;
	int i, length = xsGetArrayBufferLength(xsArg(0));
	char separator = 0;
	static const char *gHex = "0123456789ABCDEF";

	if (argc > 1) {
		char *str = xsmcToString(xsArg(1));
		separator = c_read8(str);
	}

	if (0 == length)
		xsUnknownError("0 length buffer");

	xsResult = xsStringBuffer(NULL, (length * 2) + (separator ? (length - 1) : 0));
	string = xsmcToString(xsResult);
	bytes = xsmcToArrayBuffer(xsArg(0));

	for (i = 0; i < length; i++) {
		uint8_t byte = c_read8(bytes++);
		if (separator && (0 != i))
			*string++ = separator;
		*string++ = c_read8(&gHex[byte >> 4]);
		*string++ = c_read8(&gHex[byte & 0x0F]);
	}
}

void xs_hex_toBuffer(xsMachine *the)
{
	int argc = xsmcArgc;
	char separator = 0;
	const char *string;
	uint8_t *bytes;
	int length, i;

	if (argc > 1) {
		char *str = xsmcToString(xsArg(1));
		separator = c_read8(str);
	}

	string = xsmcToString(xsArg(0));
	length = c_strlen(string);
	if (length < 2)
		xsUnknownError("string too small");

	if ((separator && ((length + 1) % 3)) || (!separator && (length % 2)))
		xsUnknownError("bad string length");

	length = separator ? ((length + 1) / 3) : (length / 2);
	xsResult = xsArrayBuffer(NULL, length);
	bytes = xsmcToArrayBuffer(xsResult);
	string = xsmcToString(xsArg(0));		// refresh
	for (i = 0; i < length; i++) {
		uint8_t c;
		uint8_t byte;

		if (separator && (0 != i)) {
			if (c_read8(string++) != separator)
				xsUnknownError("expected separator");
		}

		c = c_read8(string++);
		if (('0' <= c) && (c <= '9'))
			byte = c - '0';
		else if (('A' <= c) && (c <= 'F'))
			byte = c - 'A' + 10;
		else if (('a' <= c) && (c <= 'f'))
			byte = c - 'a' + 10;
		else
			xsUnknownError("bad data");
		byte <<= 4;

		c = c_read8(string++);
		if (('0' <= c) && (c <= '9'))
			byte |= c - '0';
		else if (('A' <= c) && (c <= 'F'))
			byte |= c - 'A' + 10;
		else if (('a' <= c) && (c <= 'f'))
			byte |= c - 'a' + 10;
		else
			xsUnknownError("bad data");

		*bytes++ = byte;
	}
}
