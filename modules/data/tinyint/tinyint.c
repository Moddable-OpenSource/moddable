/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values

void BitsView_prototype_getIntBits(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void BitsView_prototype_getUintBits(xsMachine *the)
{
	int bitsOffset = xsmcToInteger(xsArg(0));
	int bitsSize = xsmcToInteger(xsArg(1));
	uint32_t bitsValue = 0;
	int byteLength;
	uint8_t *bytes;
	uint8_t littleEndian = true;
	uint8_t shift = 0;
	static const uint8_t masks[] = {0, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

	if ((bitsSize < 1) || (bitsSize > 32))
		xsRangeError("invalid number of bits");

	if (xsmcArgc > 3) {
		if (xsmcTest(xsArg(3)))
			littleEndian = false;
	}

	xsmcVars(1);

	xsmcGet(xsVar(0), xsThis, xsID_byteLength);
	byteLength = xsmcToInteger(xsVar(0));

	if ((bitsOffset < 0) || ((bitsOffset + bitsSize) > (byteLength << 3)))
		xsRangeError("invalid offset");

	xsmcGet(xsVar(0), xsThis, xsID_buffer);
	bytes = xsmcToArrayBuffer(xsVar(0));

	xsmcGet(xsVar(0), xsThis, xsID_byteOffset);
	bytes += xsmcToInteger(xsVar(0));
	bytes += bitsOffset >> 3;
	while (bitsSize) {
		uint8_t mask;
		uint8_t part = (uint8_t)bitsSize;
		uint8_t available = 8 - (bitsOffset & 7);
		if (part > available)
			part = available;

		mask = masks[part] << (available - part);
		if (littleEndian) {
			bitsValue |= ((*bytes & mask) >> (available - part)) << shift;
			shift += part;
		}
		else {
			bitsValue <<= part;
			bitsValue |= (*bytes & mask) >> (available - part);
		}
		bytes++;

		bitsSize -= part;
		bitsOffset += part;
	}

	if (bitsValue & 0x80000000)
		xsmcSetNumber(xsResult, bitsValue);
	else
		xsmcSetInteger(xsResult, bitsValue);
}

void BitsView_prototype_setIntBits(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void BitsView_prototype_setUintBits(xsMachine *the)
{
	int bitsOffset = xsmcToInteger(xsArg(0));
	int bitsSize = xsmcToInteger(xsArg(1));
	uint32_t bitsValue = (uint32_t)xsmcToInteger(xsArg(2));
	int byteLength;
	uint8_t *bytes;
	uint8_t littleEndian = true;
	static const uint8_t masks[] = {0, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

	if ((bitsSize < 1) || (bitsSize > 32))
		xsRangeError("invalid number of bits");

	if (xsmcArgc > 3) {
		if (xsmcTest(xsArg(3))) {
			littleEndian = false;
			bitsValue <<= (32 - bitsSize);
		}
	}

	xsmcVars(1);

	xsmcGet(xsVar(0), xsThis, xsID_byteLength);
	byteLength = xsmcToInteger(xsVar(0));

	if ((bitsOffset < 0) || ((bitsOffset + bitsSize) > (byteLength << 3)))
		xsRangeError("invalid offset");

	xsmcGet(xsVar(0), xsThis, xsID_buffer);
	bytes = xsmcToArrayBuffer(xsVar(0));

	xsmcGet(xsVar(0), xsThis, xsID_byteOffset);
	bytes += xsmcToInteger(xsVar(0));
	bytes += bitsOffset >> 3;
	while (bitsSize) {
		uint8_t mask;
		uint8_t part = (uint8_t)bitsSize;
		uint8_t available = 8 - (bitsOffset & 7);
		if (part > available)
			part = available;

		mask = masks[part] << (available - part);
		if (littleEndian) {
			*bytes = (*bytes & ~mask) | ((bitsValue << (available - part)) & mask);
			bitsValue >>= part;
		}
		else {
			*bytes = (*bytes & ~mask) | ((bitsValue >> (24 + (bitsOffset & 7))) & mask);
			bitsValue <<= part;
		}
		bytes++;

		bitsSize -= part;
		bitsOffset += part;
	}
}
