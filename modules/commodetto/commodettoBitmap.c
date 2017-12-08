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


#include "commodettoBitmap.h"

#include <stdio.h>
#include <stdlib.h>

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

void xs_Bitmap_destructor(void *data)
{
}

void xs_Bitmap(xsMachine *the)
{
	int offset;
	CommodettoBitmap cb = xsmcSetHostChunk(xsThis, NULL, sizeof(CommodettoBitmapRecord));

	cb->w = (CommodettoDimension)xsmcToInteger(xsArg(0));
	cb->h = (CommodettoDimension)xsmcToInteger(xsArg(1));
	cb->format = (CommodettoBitmapFormat)xsmcToInteger(xsArg(2));
	if (kCommodettoBitmapDefault == cb->format)
		cb->format = kCommodettoBitmapFormat;
	else if (0 == cb->format)
		xsErrorPrintf("invalid bitmap format");
	offset = xsmcToInteger(xsArg(4));

	if (xsmcIsInstanceOf(xsArg(3), xsArrayBufferPrototype)) {
		cb->havePointer = false;
		cb->bits.offset = offset;
	}
	else {
		cb->havePointer = true;
		cb->bits.data = offset + (char *)xsmcGetHostData(xsArg(3));
	}

	xsmcSet(xsThis, xsID_buffer, xsArg(3));
}

void xs_bitmap_get_width(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cb->w);
}

void xs_bitmap_get_height(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cb->h);
}

void xs_bitmap_get_pixelFormat(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cb->format);
}

void xs_bitmap_get_offset(xsMachine *the)
{
	CommodettoBitmap cb = xsmcGetHostChunk(xsThis);
	int32_t offset;

	if (cb->havePointer) {
		xsmcGet(xsResult, xsThis, xsID_buffer);
		offset = (char *)cb->bits.data - (char *)xsmcGetHostData(xsResult);
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
	else if ((kCommodettoBitmapGray256 == format) || (kCommodettoBitmapRGB332 == format))
		depth = 8;
	else if ((kCommodettoBitmapRGB565LE == format) || (kCommodettoBitmapRGB565BE == format))
		depth = 16;
	else if (kCommodettoBitmap24RGB == format)
		depth = 24;
	else if (kCommodettoBitmap32RGBA == format)
		depth = 32;

	return depth;
}
