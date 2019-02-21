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
#include "mc.xs.h"			// for xsID_ values

void xs_parseBMF(xsMachine *the)
{
	unsigned char *bytes, *start;
	uint32_t size;
	int charCount;

	xsmcVars(1);

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		bytes = xsmcToArrayBuffer(xsArg(0));
	else
		bytes = xsmcGetHostData(xsArg(0));
	start = bytes;

	if ((0x42 != c_read8(bytes + 0)) || (0x4D != c_read8(bytes + 1)) || (0x46 != c_read8(bytes + 2)) || ((3 != c_read8(bytes + 3)) && (4 != c_read8(bytes + 3))))
		xsUnknownError("Invalid BMF header");
	bytes += 4;

	// skip block 1
	if (1 != c_read8(bytes))
		xsUnknownError("can't find info block");
	bytes += 1;

	bytes += 4 + c_read32(bytes);

	// get lineHeight from block 2
	if (2 != c_read8(bytes))
		xsUnknownError("can't find common block");
	bytes += 1;

	size = c_read32(bytes);
	bytes += 4;

	xsmcSetInteger(xsVar(0), c_read16(bytes));
	bytes += 2;
	xsmcSet(xsArg(0), xsID_height, xsVar(0));

	xsmcSetInteger(xsVar(0), c_read16(bytes));
	bytes += 2;
	xsmcSet(xsArg(0), xsID_ascent, xsVar(0));

	bytes += 2 + 2;		// scaleW and scaleH
	if (1 != c_read16(bytes))	// pages
		xsUnknownError("not single page");

	bytes += size - 8;

	// skip block 3
	if (3 != c_read8(bytes))
		xsUnknownError("can't find pages block");
	bytes += 1;

	bytes += 4 + c_read32(bytes);

	// use block 4
	if (4 != c_read8(bytes))
		xsUnknownError("can't find chars block");
	bytes += 1;

	xsmcSetInteger(xsVar(0), bytes - start);
	xsmcSet(xsArg(0), xsID_position, xsVar(0));		// position of size of chars table

	size = c_read32(bytes);
	if (size % 20)
		xsUnknownError("bad chars block size");
	charCount = size / 20;

	xsmcSetInteger(xsVar(0), charCount);
	xsmcSet(xsArg(0), xsID_charCount, xsVar(0));

	xsResult = xsArg(0);
}
