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

#include "xsPlatform.h"
#include "xsmc.h"

static void hexStringToBytes(xsMachine *the, uint8_t *buffer, const char *string, uint16_t length, uint8_t reverse)
{
	uint8_t c, byte, *dst;
	int8_t bump;
	uint16_t i;
	
	length /= 2;
	if (reverse) {
		dst = buffer + length - 1;
		bump = -1;
	}
	else {
		dst = buffer;
		bump = +1;
	}
	
	for (i = 0; i < length; i++) {
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
		*dst = byte;
		dst += bump;
	}
}

void xs_bytes_set(xsMachine *the)
{
	uint8_t reverse = xsmcToBoolean(xsArg(1));
	if (xsmcTypeOf(xsArg(0)) == xsStringType) {
		const char *s = xsmcToString(xsArg(0));
		hexStringToBytes(the, xsmcToArrayBuffer(xsThis), s, c_strlen(s), reverse);
	}
	else {
		uint8_t *src = xsmcToArrayBuffer(xsArg(0));
		uint8_t *dst = xsmcToArrayBuffer(xsThis);
		uint16_t i, length = xsGetArrayBufferLength(xsArg(0));
		if (reverse)
			for (i = 0; i < length; ++i)
				dst[i] = src[length - i - 1];
		else
			c_memmove(dst, src, length);
	}
}

void xs_bytes_equals(xsMachine *the)
{
	uint16_t length = xsGetArrayBufferLength(xsArg(0));
	if (length != xsGetArrayBufferLength(xsThis))
		xsmcSetFalse(xsResult);
	else {
		uint16_t result = c_memcmp(xsmcToArrayBuffer(xsThis), xsmcToArrayBuffer(xsArg(0)), length);
		xsmcSetBoolean(xsResult, (0 == result));
	}
}
