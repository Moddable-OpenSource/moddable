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

#define kUUIDSeparator '-'

static void hexStringToBytes(xsMachine *the, uint8_t *buffer, const char *string, uint16_t length)
{
	uint8_t c, byte;
	uint16_t i;
	
	for (i = 0, length /= 2; i < length; i++) {
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
		*buffer++ = byte;
	}
}

static void bytesToHexString(xsMachine *the, char *string, uint8_t *buffer, uint16_t length)
{
	uint16_t i;
	static const char *gHex = "0123456789ABCDEF";
	for (i = 0; i < length; i++) {
		uint8_t byte = c_read8(buffer++);
		*string++ = c_read8(&gHex[byte >> 4]);
		*string++ = c_read8(&gHex[byte & 0x0F]);
	}
}

void xs_btuuid_toString(xsMachine *the)
{
	int i, length = xsGetArrayBufferLength(xsArg(0));
	uint8_t byte, *bytes = xsmcToArrayBuffer(xsArg(0));
	char *string;

	if (2 == length) {
		xsResult = xsStringBuffer(NULL, length * 2);
		string = xsmcToString(xsResult);
		bytesToHexString(the, string, bytes, 2);
	}
	else if (16 == length) {
		xsResult = xsStringBuffer(NULL, 36);
		string = xsmcToString(xsResult);
		bytesToHexString(the, string, bytes, 4);
		bytes += 4; string += 8; *string++ = kUUIDSeparator;
		bytesToHexString(the, string, bytes, 2);
		bytes += 2; string += 4; *string++ = kUUIDSeparator;
		bytesToHexString(the, string, bytes, 2);
		bytes += 2; string += 4; *string++ = kUUIDSeparator;
		bytesToHexString(the, string, bytes, 2);
		bytes += 2; string += 4; *string++ = kUUIDSeparator;
		bytesToHexString(the, string, bytes, 6);
	}
	else
		xsUnknownError("invalid uuid length");
}

void xs_btuuid_toBuffer(xsMachine *the)
{
	const char *uuid = xsmcToString(xsArg(0));
	uint16_t byteLen, strLen = c_strlen(uuid);
	uint8_t buffer[16];
	if (4 == strLen) {
		hexStringToBytes(the, buffer, uuid, strLen);
		byteLen = strLen / 2;
	}
	else if (36 == strLen) {
		hexStringToBytes(the, &buffer[0], &uuid[0], 8);
		hexStringToBytes(the, &buffer[4], &uuid[9], 4);
		hexStringToBytes(the, &buffer[6], &uuid[14], 4);
		hexStringToBytes(the, &buffer[8], &uuid[19], 4);
		hexStringToBytes(the, &buffer[10], &uuid[24], 12);
		byteLen = 16;
	}
	else
		xsUnknownError("invalid uuid length");

	xsmcSetArrayBuffer(xsResult, buffer, byteLen);
}
