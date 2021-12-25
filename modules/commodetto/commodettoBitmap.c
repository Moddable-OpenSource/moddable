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


#include "commodettoBitmap.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#if COMMODETTO_BITMAP_ID
	static uint32_t gBitmapID;
#endif

void xs_Bitmap_destructor(void *data)
{
}

void xs_Bitmap(xsMachine *the)
{
	int offset;
	int32_t byteLength = (xsmcArgc > 5) ? xsmcToInteger(xsArg(5)) : 0;
	CommodettoBitmapRecord cb;
	void *data;
	xsUnsignedValue dataSize, neededSize;

	cb.w = (CommodettoDimension)xsmcToInteger(xsArg(0));
	cb.h = (CommodettoDimension)xsmcToInteger(xsArg(1));
	cb.format = (CommodettoBitmapFormat)xsmcToInteger(xsArg(2));
	if (kCommodettoBitmapDefault == cb.format)
		cb.format = kCommodettoBitmapFormat;
	else if (0 == cb.format)
		xsErrorPrintf("invalid format");
	offset = xsmcToInteger(xsArg(4));

	if (xsBufferRelocatable == xsmcGetBufferReadable(xsArg(3), (void **)&data, &dataSize)) {
		cb.havePointer = false;
		cb.bits.offset = offset;
	}
	else {
		cb.havePointer = true;
		cb.bits.data = offset + (char *)data;
	}

	if (kCommodettoBitmapPacked & cb.format)
		neededSize = 0;
	else
		neededSize = (((CommodettoBitmapGetDepth(cb.format) * cb.w) + 7) >> 3) * cb.h;
	if ((offset < 0) || (((xsUnsignedValue)offset + neededSize) > dataSize))
		xsErrorPrintf("invalid");

	#if COMMODETTO_BITMAP_ID
		cb.id = ++gBitmapID;
	#endif

	if (byteLength) {
		cb.flags |= kCommodettoBitmapHaveByteLength;
		cb.byteLength = byteLength;
	}

	xsmcSetHostChunk(xsThis, &cb, sizeof(CommodettoBitmapRecord) /* - (byteLength ? sizeof(int32_t) : 0) */);

	xsmcDefine(xsThis, xsID_buffer, xsArg(3), xsDontSet);
}

void xs_bitmap_get_width(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunkValidate(xsThis, xs_Bitmap_destructor);
	xsmcSetInteger(xsResult, cb->w);
}

void xs_bitmap_get_height(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunkValidate(xsThis, xs_Bitmap_destructor);
	xsmcSetInteger(xsResult, cb->h);
}

void xs_bitmap_get_pixelFormat(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunkValidate(xsThis, xs_Bitmap_destructor);
	xsmcSetInteger(xsResult, cb->format);
}

void xs_bitmap_get_offset(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunkValidate(xsThis, xs_Bitmap_destructor);
	int32_t offset;

	if (cb->havePointer) {
		void *data;
		xsUnsignedValue dataSize;

		xsmcGet(xsResult, xsThis, xsID_buffer);
		xsmcGetBufferReadable(xsResult, &data, &dataSize);
		offset = (char *)cb->bits.data - (char *)data;
	}
	else
		offset = cb->bits.offset;

	xsmcSetInteger(xsResult, offset);
}

void xs_bitmap_get_depth(xsMachine *the)
{
	int format = xsmcToInteger(xsArg(0));
	int depth = CommodettoBitmapGetDepth((CommodettoBitmapFormat)format);

	if (0 == depth)
		xsErrorPrintf("unrecognized bitmap format");

	xsmcSetInteger(xsResult, depth);
}

uint8_t CommodettoBitmapGetDepth(CommodettoBitmapFormat format)
{
	int depth = 0;

	format &= ~kCommodettoBitmapPacked;

	if (kCommodettoBitmapMonochrome == format)
		depth = 1;
	else if ((kCommodettoBitmapGray16 == format) || (kCommodettoBitmapCLUT16 == format))
		depth = 4;
	else if ((kCommodettoBitmapGray256 == format) || (kCommodettoBitmapRGB332 == format) || (kCommodettoBitmapCLUT256 == format))
		depth = 8;
	else if ((kCommodettoBitmapRGB565LE == format) || (kCommodettoBitmapRGB565BE == format) || (kCommodettoBitmapARGB4444 == format))
		depth = 16;
	else if (kCommodettoBitmap24RGB == format)
		depth = 24;
	else if (kCommodettoBitmap32RGBA == format)
		depth = 32;
	else if (kCommodettoBitmapRGB444 == format)
		depth = 12;
	else if (kCommodettoBitmapCLUT32 == format)
		depth = 5;

	return depth;
}
