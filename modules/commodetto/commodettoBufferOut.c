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


/*
	Commodetto Buffer in Memory
		
		implements PixelOut to write pixels to an ArrayBuffer
*/


#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

#include "string.h"
#include "stdint.h"
#include "stdlib.h"

#include "commodettoBitmap.h"

typedef struct {
	CommodettoBitmapFormat	pixelFormat;
	uint8_t					depth;

	CommodettoDimension		width;
	CommodettoDimension		height;

	CommodettoDimension		windowWidth;
	int						windowOffset;
	int						offset;
	int						rowBump;
} xsBufferOutRecord, *xsBufferOut;

#define pixelsToBytes(count) ((((count) * (bo->depth)) + 7) >> 3)

void xs_BufferOut_destructor(void *data)
{
	if (data)
		free(data);
}

void xs_BufferOut_init(xsMachine *the)
{
	xsBufferOut bo;

	bo = calloc(sizeof(xsBufferOutRecord), 1);
	if (!bo)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, bo);
	xsmcVars(2);

	bo->width = xsmcToInteger(xsArg(0));
	bo->height = xsmcToInteger(xsArg(1));
	bo->pixelFormat = (CommodettoBitmapFormat)xsmcToInteger(xsArg(2));
	bo->depth = CommodettoBitmapGetDepth(bo->pixelFormat);

	if (xsmcTest(xsArg(3)))
		xsVar(0) = xsArg(3);
	else
		xsmcSetArrayBuffer(xsVar(0), NULL, pixelsToBytes(bo->width) * bo->height);
	xsmcDefine(xsThis, xsID_buffer, xsVar(0), xsDontSet);
}

void xs_BufferOut_begin(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	int x = xsmcToInteger(xsArg(0));
	int y = xsmcToInteger(xsArg(1));
	int width = xsmcToInteger(xsArg(2));

	bo->offset = pixelsToBytes((bo->width * y) + x);
	bo->windowWidth = pixelsToBytes(width);
	bo->windowOffset = 0;
	bo->rowBump = pixelsToBytes(bo->width - width);
}

void xs_BufferOut_send(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	char *src, *dst;
	int offsetIn, count, offsetOut;
	xsUnsignedValue available;
	xsSlot bufferSlot;

	offsetIn = (argc > 1) ? xsmcToInteger(xsArg(1)) : 0;

	if (argc > 2)
		count = xsmcToInteger(xsArg(2));

	xsmcGetBufferReadable(xsArg(0), (void **)&src, &available);
	if (((xsUnsignedValue)offsetIn >= available) || (offsetIn < 0) || ((xsUnsignedValue)(offsetIn + count) > available) || (count <= 0))
		xsUnknownError("invalid");
	if (argc <= 2)
		count = available - offsetIn;

	xsmcGet(bufferSlot, xsThis, xsID_buffer);
	xsmcGetBufferWritable(bufferSlot, (void **)&dst, &available);

	offsetOut = bo->offset;

	while (count) {
		int copy = bo->windowWidth - bo->windowOffset;
		if (copy > count)
			copy = count;
		count -= copy;
		bo->windowOffset += copy;
		c_memcpy(dst + offsetOut, src + offsetIn, copy);
		offsetOut += copy;
		offsetIn += copy;
		if (bo->windowOffset == bo->windowWidth) {
			bo->windowOffset = 0;
			offsetOut += bo->rowBump;
		}
	}

	bo->offset = offsetOut;
}

void xs_BufferOut_pixelsToBytes(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	int count = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, pixelsToBytes(count));
}

void xs_BufferOut_getWidth(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, bo->width);
}

void xs_BufferOut_getHeight(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, bo->height);
}

void xs_BufferOut_getPixelFormat(xsMachine *the)
{
	xsBufferOut bo = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, bo->pixelFormat);
}

