/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

//@@ can read beyond end of provided buffer on malformed data
void xs_parseBMF(xsMachine *the)
{
	unsigned char *bytes, *start;
	uint32_t size;
	xsUnsignedValue byteLength;
	int charCount;

	xsmcGetBufferReadable(xsArg(0), (void **)&bytes, &byteLength);
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

	xsmcSetInteger(xsResult, c_read16(bytes));
	bytes += 2;
	xsmcDefine(xsArg(0), xsID_height, xsResult, xsDontDelete | xsDontSet);

	xsmcSetInteger(xsResult, c_read16(bytes));
	bytes += 2;
	xsmcDefine(xsArg(0), xsID_ascent, xsResult, xsDontDelete | xsDontSet);

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

	xsmcSetInteger(xsResult, bytes - start);
	xsmcDefine(xsArg(0), xsID_position, xsResult, xsDontDelete | xsDontSet);

	size = c_read32(bytes);
	if (size % 20)
		xsUnknownError("bad chars block size");
	charCount = size / 20;

	xsmcSetInteger(xsResult, charCount);
	xsmcDefine(xsArg(0), xsID_charCount, xsResult, xsDontDelete | xsDontSet);

	xsResult = xsArg(0);
}
