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


#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"

#include "stddef.h"		// for offsetof macro
#include "stdint.h"
#include "stdlib.h"

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#ifdef __ets__				//@@ move somewhere else!
	#include "xsesp.h"
#else
	#undef c_read8
	#undef c_read16
	#undef c_read32
	#define c_read8(x) (*(uint8_t *)(x))
	#define c_read16(x) (*(uint16_t *)(x))
	#define c_read32(x) (*(uint32_t *)(x))
#endif

#define xsGetHostDataPoco(slot) ((void *)((char *)xsmcGetHostData(slot) - offsetof(PocoRecord, pixels)))

#define PocoDisableGC(poco) \
	if (!(poco->flags & kPocoFlagGCDisabled)) {	\
		poco->flags |= kPocoFlagGCDisabled;	\
		xsEnableGarbageCollection(false);	\
	}

void xs_poco_destructor(void *data)
{
	if (data)
		free(((uint8_t *)data) - offsetof(PocoRecord, pixels));
}

void xs_poco_build(xsMachine *the)
{
	Poco poco;
	int pixelsLength = xsmcToInteger(xsArg(2));
	int pixelFormat = xsmcToInteger(xsArg(3));
	int displayListLength = xsmcToInteger(xsArg(4));
	int rotation = xsmcToInteger(xsArg(5));
	int byteLength = pixelsLength + displayListLength;

	if (kPocoPixelFormat != pixelFormat)
		xsErrorPrintf("unsupported pixel format");

	if (rotation != kPocoRotation)
		xsErrorPrintf("not configured for requested rotation");

	poco = malloc(sizeof(PocoRecord) + byteLength + 8);		// overhang when dividing
	if (!poco)
		xsErrorPrintf("out of menory");
	xsmcSetHostData(xsThis, poco->pixels);

	poco->width = (PocoDimension)xsmcToInteger(xsArg(0));
	poco->height = (PocoDimension)xsmcToInteger(xsArg(1));
	rotateDimensions(poco->width, poco->height);

	poco->flags = 0;
	if (xsmcTest(xsArg(6)))
		poco->flags |= kPocoFlagDoubleBuffer;
	if (xsmcTest(xsArg(7)))
		poco->flags |= kPocoFlagAdaptInvalid;

	if (xsmcTest(xsArg(8))) {
		PixelsOutDispatch pixelsOutDispatch;
		poco->outputRefcon = xsmcGetHostData(xsArg(8));
		pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;
		if (pixelsOutDispatch && pixelsOutDispatch->doAdaptInvalid)
			poco->flags |= kPocoFlagAdaptInvalid;
		else
			poco->flags &= ~kPocoFlagAdaptInvalid;
	}
	else
		poco->outputRefcon = NULL;

	if (xsmcTest(xsArg(9)))
		poco->flags |= kPocoFlagFrameBuffer;

#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	if (xsmcTest(xsArg(10)))
		poco->clut = xsmcGetHostData(xsArg(10));
#endif

	poco->pixelsLength = pixelsLength;

	poco->displayList = ((char *)poco->pixels) + pixelsLength;
	poco->displayListEnd = poco->displayList + displayListLength;

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), pixelsLength);
	xsmcSet(xsThis, xsID_byteLength, xsVar(0));
}

void xs_poco_begin(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoCoordinate x, y;
	PocoDimension w, h;

	if (1 == argc) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_x);
		x = (PocoCoordinate)xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(0), xsID_y);
		y = (PocoCoordinate)xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(0), xsID_w);
		w = (PocoDimension)xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(0), xsID_h);
		h = (PocoDimension)xsmcToInteger(xsVar(0));
	}
	else {
		if (argc > 1)
			x = (PocoCoordinate)xsmcToInteger(xsArg(0));
		else
			x = 0;

		if (argc >= 2)
			y = (PocoCoordinate)xsmcToInteger(xsArg(1));
		else
			y = 0;

		if (argc >= 3)
			w = (PocoDimension)xsmcToInteger(xsArg(2));
		else
			w = poco->width - x;

		if (argc >= 4)
			h = (PocoDimension)xsmcToInteger(xsArg(3));
		else
			h = poco->height - y;
	}

#if !kPocoFrameBuffer
	PocoDrawingBegin(poco, x, y, w, h);
#else
	if (!(poco->flags & kPocoFlagFrameBuffer))
		PocoDrawingBegin(poco, x, y, w, h);
	else {
		PocoPixel *pixels;
		int16_t rowBytes;
		PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;

		if (pixelsOutDispatch)
			(pixelsOutDispatch->doBeginFrameBuffer)(poco->outputRefcon, &pixels, &rowBytes);
		else {
			xsmcVars(5);
			xsmcGet(xsVar(0), xsThis, xsID_pixelsOut);
			xsmcSetInteger(xsVar(1), poco->x);
			xsmcSetInteger(xsVar(2), poco->y);
			xsmcSetInteger(xsVar(3), poco->w);
			xsmcSetInteger(xsVar(4), poco->h);
			xsResult = xsCall4(xsVar(0), xsID_begin, xsVar(1), xsVar(2), xsVar(3), xsVar(4));
			pixels = xsmcGetHostData(xsResult);

			xsmcGet(xsVar(0), xsResult, xsID_byteLength);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
			rowBytes = (int16_t)(xsmcToInteger(xsVar(0)) / poco->height);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
			rowBytes = (int16_t)(xsmcToInteger(xsVar(0)) / poco->width);
#endif
		}

		PocoDrawingBeginFrameBuffer(poco, x, y, w, h, pixels, rowBytes);
	}
#endif

	poco->flags &= ~kPocoFlagGCDisabled;
}

static void pixelReceiver(PocoPixel *pixels, int byteLength, void *refCon)
{
	xsMachine *the = refCon;
	Poco poco = xsGetHostDataPoco(xsThis);		//@@ eliminate this call

	xsmcSetInteger(xsVar(1), (char *)pixels - (char *)poco->pixels);		// offset
	xsmcSetInteger(xsVar(2), byteLength);
	xsCall3(xsVar(0), xsID_send, xsVar(3), xsVar(1), xsVar(2));
}

void xs_poco_end(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;

	if (!(poco->flags & kPocoFlagFrameBuffer)) {
		int result;

		xsmcVars(5);

		if (pixelsOutDispatch) {
			(pixelsOutDispatch->doBegin)(poco->outputRefcon, poco->x, poco->y, poco->w, poco->h);
		}
		else {
			xsmcGet(xsVar(0), xsThis, xsID_pixelsOut);
			xsmcSetInteger(xsVar(1), poco->x);
			xsmcSetInteger(xsVar(2), poco->y);
			xsmcSetInteger(xsVar(3), poco->w);
			xsmcSetInteger(xsVar(4), poco->h);
			xsCall4(xsVar(0), xsID_begin, xsVar(1), xsVar(2), xsVar(3), xsVar(4));
		}

		if (poco->outputRefcon)
			result = PocoDrawingEnd(poco, poco->pixels, poco->pixelsLength, pixelsOutDispatch->doSend, poco->outputRefcon);
		else {
			xsVar(3) = xsThis;		// Poco doubles as pixels
			result = PocoDrawingEnd(poco, poco->pixels, poco->pixelsLength, pixelReceiver, the);
		}
		if (result) {
			if (1 == result)
				xsErrorPrintf("display list overflowed");
			if (2 == result)
				xsErrorPrintf("clip/origin stack not cleared");
			if (3 == result)
				xsErrorPrintf("clip/origin stack under/overflow");
			xsErrorPrintf("unknown error");
		}
	}
	else {
#if kPocoFrameBuffer
		PocoDrawingEndFrameBuffer(poco);

		if (NULL == pixelsOutDispatch) {
			xsmcVars(1);
			xsmcGet(xsVar(0), xsThis, xsID_pixelsOut);
		}
#else
		xsErrorPrintf("frameBuffer disabled");
#endif
	}

	if (poco->flags & kPocoFlagGCDisabled) {
		xsEnableGarbageCollection(true);
		xsCollectGarbage();
		poco->flags &= ~kPocoFlagGCDisabled;
	}

	if ((xsmcArgc > 0) && xsmcTest(xsArg(0))) {
		if (pixelsOutDispatch)
			(pixelsOutDispatch->doContinue)(poco->outputRefcon);
		else
			xsCall0(xsVar(0), xsID_continue);
	}
	else {
		if (pixelsOutDispatch)
			(pixelsOutDispatch->doEnd)(poco->outputRefcon);
		else
			xsCall0(xsVar(0), xsID_end);

		pocoInstrumentationAdjust(FramesDrawn, 1);
	}
}

void xs_poco_makeColor(xsMachine *the)
{
	int r, g, b, color;

	r = xsmcToInteger(xsArg(0));
	g = xsmcToInteger(xsArg(1));
	b = xsmcToInteger(xsArg(2));

	color = PocoMakeColor(r, g, b);

	xsmcSetInteger(xsResult, color);
}

void xs_poco_clip(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;

	if (argc) {
		PocoCoordinate x, y;
		PocoDimension w, h;

		x = (PocoCoordinate)xsmcToInteger(xsArg(0)) + poco->xOrigin;
		y = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->yOrigin;
		w = (PocoDimension)xsmcToInteger(xsArg(2));
		h = (PocoDimension)xsmcToInteger(xsArg(3));
		PocoClipPush(poco, x, y, w, h);
	}
	else
		PocoClipPop(poco);
}

void xs_poco_origin(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;

	if (argc) {
		PocoCoordinate x, y;

		x = (PocoCoordinate)xsmcToInteger(xsArg(0));
		y = (PocoCoordinate)xsmcToInteger(xsArg(1));
		PocoOriginPush(poco, x, y);
	}
	else
		PocoOriginPop(poco);
}

void xs_poco_fillRectangle(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PocoColor color;
	PocoCoordinate x, y;
	PocoDimension w, h;

	color = (PocoColor)xsmcToInteger(xsArg(0));
	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	w = (PocoDimension)xsmcToInteger(xsArg(3));
	h = (PocoDimension)xsmcToInteger(xsArg(4));

	PocoRectangleFill(poco, color, kPocoOpaque, x, y, w, h);
}

void xs_poco_blendRectangle(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PocoColor color;
	PocoCoordinate x, y;
	PocoDimension w, h;
	uint8_t blend;

	color = (PocoColor)xsmcToInteger(xsArg(0));
	blend = (uint8_t)xsmcToInteger(xsArg(1));
	x = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->yOrigin;
	w = (PocoDimension)xsmcToInteger(xsArg(4));
	h = (PocoDimension)xsmcToInteger(xsArg(5));

	PocoRectangleFill(poco, color, blend, x, y, w, h);
}

void xs_poco_drawPixel(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PocoColor color;
	PocoCoordinate x, y;

	color = (PocoColor)xsmcToInteger(xsArg(0));
	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;

	PocoPixelDraw(poco, color, x, y);
}

void xs_poco_drawBitmap(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	CommodettoBitmap cb;

	xsmcVars(1);

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;

	if (cb->havePointer)
		bits.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(0), xsID_buffer);
		bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;

	sx = 0, sy = 0;
#if (90 == kPocoRotation) || (270 == kPocoRotation)
	sw = bits.height, sh = bits.width;
#else
	sw = bits.width, sh = bits.height;
#endif

	if (argc > 3) {
		sx = (PocoCoordinate)xsmcToInteger(xsArg(3));
		sy = (PocoCoordinate)xsmcToInteger(xsArg(4));
		if (argc > 5) {
			sw = (PocoDimension)xsmcToInteger(xsArg(5));
			sh = (PocoDimension)xsmcToInteger(xsArg(6));
		}
	}

	PocoBitmapDraw(poco, &bits, x, y, sx, sy, sw, sh);
}

void xs_poco_drawMonochrome(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	PocoColor fgColor = 0, bgColor = 0;
	PocoMonochromeMode mode = 0;
	CommodettoBitmap cb;

	xsmcVars(1);

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;

	if (cb->havePointer)
		bits.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(0), xsID_buffer);
		bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	if (xsUndefinedType != xsmcTypeOf(xsArg(1))) {
		fgColor = (PocoColor)xsmcToInteger(xsArg(1));
		mode = kPocoMonochromeForeground;
	}

	if (xsUndefinedType != xsmcTypeOf(xsArg(2))) {
		bgColor = (PocoColor)xsmcToInteger(xsArg(2));
		mode |= kPocoMonochromeBackground;
	}

	x = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(4)) + poco->yOrigin;

	sx = 0, sy = 0;
	sw = bits.width, sh = bits.height;

	if (argc > 5) {
		sx = (PocoDimension)xsmcToInteger(xsArg(5));
		sy = (PocoDimension)xsmcToInteger(xsArg(6));
		if (argc > 7) {
			sw = (PocoDimension)xsmcToInteger(xsArg(7));
			sh = (PocoDimension)xsmcToInteger(xsArg(8));
		}
	}

	PocoMonochromeBitmapDraw(poco, &bits, mode, fgColor, bgColor, x, y, sx, sy, sw, sh);
}

void xs_poco_drawGray(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	PocoColor color = 0;
	CommodettoBitmap cb;
	uint8_t blend = kPocoOpaque;

	xsmcVars(1);

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;

	if (cb->havePointer)
		bits.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(0), xsID_buffer);
		bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	color = (PocoColor)xsmcToInteger(xsArg(1));

	x = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->yOrigin;

	sx = 0, sy = 0;
#if (90 == kPocoRotation) || (270 == kPocoRotation)
	sw = bits.height, sh = bits.width;
#else
	sw = bits.width, sh = bits.height;
#endif

	if (argc > 4) {
		sx = (PocoDimension)xsmcToInteger(xsArg(4));
		sy = (PocoDimension)xsmcToInteger(xsArg(5));
		if (argc > 6) {
			sw = (PocoDimension)xsmcToInteger(xsArg(6));
			sh = (PocoDimension)xsmcToInteger(xsArg(7));

			if (argc > 8)
				blend = (uint8_t)xsmcToInteger(xsArg(8));
		}
	}

	PocoGrayBitmapDraw(poco, &bits, color, blend, x, y, sx, sy, sw, sh);
}

void xs_poco_drawMasked(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits, mask;
	PocoCoordinate x, y, mask_sx, mask_sy;
	PocoDimension sx, sy, sw, sh;
	CommodettoBitmap cb;

	xsmcVars(1);

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;

	if (cb->havePointer)
		bits.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(0), xsID_buffer);
		bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	sx = (PocoCoordinate)xsmcToInteger(xsArg(3));
	sy = (PocoCoordinate)xsmcToInteger(xsArg(4));
	sw = (PocoDimension)xsmcToInteger(xsArg(5));
	sh = (PocoDimension)xsmcToInteger(xsArg(6));

	cb = xsmcGetHostChunk(xsArg(7));
	mask.width = cb->w;
	mask.height = cb->h;
	mask.format = cb->format;

	if (cb->havePointer)
		mask.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(7), xsID_buffer);
		mask.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	mask_sx = (PocoCoordinate)xsmcToInteger(xsArg(8));
	mask_sy = (PocoCoordinate)xsmcToInteger(xsArg(9));

	if (argc > 10) {
		uint8_t blend = (uint8_t)xsmcToInteger(xsArg(10));
		PocoBitmapDrawMasked(poco, blend, &bits, x, y, sx, sy, sw, sh, &mask, mask_sx, mask_sy);
	}
	else
		PocoBitmapDrawMasked(poco, kPocoOpaque, &bits, x, y, sx, sy, sw, sh, &mask, mask_sx, mask_sy);
}

void xs_poco_fillPattern(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension w, h, sx, sy, sw, sh;
	CommodettoBitmap cb;

	xsmcVars(1);

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;

	if (cb->havePointer)
		bits.pixels = cb->bits.data;
	else {
		xsmcGet(xsVar(0), xsArg(0), xsID_buffer);
		bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(0)) + cb->bits.offset);
		PocoDisableGC(poco);
	}

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	w = (PocoDimension)xsmcToInteger(xsArg(3));
	h = (PocoDimension)xsmcToInteger(xsArg(4));
	if (argc > 5) {
		sx = (PocoCoordinate)xsmcToInteger(xsArg(5));
		sy = (PocoCoordinate)xsmcToInteger(xsArg(6));
		sw = (PocoDimension)xsmcToInteger(xsArg(7));
		sh = (PocoDimension)xsmcToInteger(xsArg(8));
		PocoBitmapPattern(poco, &bits, x, y, w, h, sx, sy, sw, sh);
	}
	else
		PocoBitmapPattern(poco, &bits, x, y, w, h, 0, 0, bits.width, bits.height);
}

void xs_poco_drawFrame(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	uint8_t *data;
	uint32_t dataSize;
	PocoCoordinate x, y;
	PocoDimension w, h;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		data = xsmcToArrayBuffer(xsArg(0));
		dataSize = xsGetArrayBufferLength(xsArg(0));
		PocoDisableGC(poco);
	}
	else {
		data = (uint8_t *)xsmcGetHostData(xsArg(0));
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
		dataSize = xsmcToInteger(xsVar(0));
	}

	x = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->yOrigin;

	xsmcGet(xsResult, xsArg(1), xsID_width);
	w = xsmcToInteger(xsResult);
	xsmcGet(xsResult, xsArg(1), xsID_height);
	h = xsmcToInteger(xsResult);

	PocoDrawFrame(poco, data, dataSize, x, y, w, h);
}

void xs_poco_getTextWidth(xsMachine *the)
{
	const unsigned char *text = (const unsigned char *)xsmcToString(xsArg(0));
	const unsigned char *chars;
	int charCount, width = 0;

	if (xsmcIsInstanceOf(xsArg(1), xsArrayBufferPrototype))
		chars = xsmcToArrayBuffer(xsArg(1));
	else
		chars = xsmcGetHostData(xsArg(1));

	xsmcGet(xsResult, xsArg(1), xsID_position);
	chars += xsmcToInteger(xsResult);

	charCount = c_read32(chars) / 20;
	chars += 4;

	while (true) {
		const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t **)&text, chars, charCount);
		if (!cc) {
			if (!c_read8(text - 1))
				break;
			continue;
		}

		width += c_read16(cc + 16);	// +16 -> offset to xadvance
	}

	xsmcSetInteger(xsResult, width);
}

void xs_poco_drawText(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	const unsigned char *text = (const unsigned char *)xsmcToString(xsArg(0));
	int charCount;
	PocoCoordinate x, y;
	PocoColor color;
	CommodettoBitmap cb;
	PocoBitmapRecord bits;
	static const char *ellipsis = "...";
	PocoDimension ellipsisWidth;
	int width;
	const unsigned char *chars;
	uint8_t isColor, isCompressed;
	PocoBitmapRecord mask;

	x = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(4)) + poco->yOrigin;

	xsmcVars(2);

	if (xsmcIsInstanceOf(xsArg(1), xsArrayBufferPrototype))
		chars = xsmcToArrayBuffer(xsArg(1));
	else
		chars = xsmcGetHostData(xsArg(1));

	isCompressed = 4 == c_read8(chars + 3);

	xsmcGet(xsVar(0), xsArg(1), xsID_position);
	chars += xsmcToInteger(xsVar(0));

	charCount = c_read32(chars) / 20;
	chars += 4;

	if (argc > 5) {
		const char *t = ellipsis;
		const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t **)&t, chars, charCount);

		width = xsmcToInteger(xsArg(5));
		if (cc) {
			ellipsisWidth = c_read16(cc + 16);						// +16 -> offset to xadvance
			ellipsisWidth *= 3;
		}
		else
			ellipsisWidth = 0;
	}
	else {
		width = 0;
		ellipsisWidth = 0;
	}

	if (!isCompressed) {
		// fill out the bitmap
		xsmcGet(xsVar(0), xsArg(1), xsID_bitmap);
		cb = xsmcGetHostChunk(xsVar(0));
		bits.width = cb->w;
		bits.height = cb->h;
		bits.format = cb->format;

		if (cb->havePointer)
			bits.pixels = cb->bits.data;
		else {
			xsmcGet(xsVar(1), xsVar(0), xsID_buffer);
			bits.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(1)) + cb->bits.offset);
			PocoDisableGC(poco);
		}

		if (kPocoPixelFormat == cb->format) {
			isColor = 1;
			if (xsReferenceType == xsmcTypeOf(xsArg(2))) {
				isColor = 2;

				cb = xsmcGetHostChunk(xsArg(2));
				mask.width = cb->w;
				mask.height = cb->h;
				mask.format = cb->format;
				if (cb->havePointer)
					mask.pixels = cb->bits.data;
				else {
					xsmcGet(xsVar(1), xsArg(2), xsID_buffer);
					mask.pixels = (PocoPixel *)((char *)xsmcToArrayBuffer(xsVar(1)) + cb->bits.offset);
					PocoDisableGC(poco);
				}
			}
		}
		else {
			isColor = 0;
			color = (PocoColor)xsmcToInteger(xsArg(2));
		}
	}
	else {
		bits.format = kCommodettoBitmapGray16 | kCommodettoBitmapPacked;

		isColor = 0;
		color = (PocoColor)xsmcToInteger(xsArg(2));
	}

	while (true) {
		PocoCoordinate cx, cy, sx, sy;
		PocoDimension sw, sh;
		uint16_t xadvance;
		const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t **)&text, chars, charCount);
		if (!cc) {
			if (!c_read8(text - 1))
				break;
			continue;
		}

		xadvance = c_read16(cc + 16);	// +16 -> offset to xadvance

		if (ellipsisWidth && ((width - xadvance) <= ellipsisWidth)) {
			// measure the rest of the string to see if it fits
			const unsigned char *t = text - 1;		//@@ fails if multibyte...
			int w = 0;
			while (w < width) {
				const uint8_t *tc = PocoBMFGlyphFromUTF8((uint8_t **)&t, chars, charCount);
				if (!tc) {
					if (!c_read8(t - 1))
						break;
					continue;
				}

				w += c_read16(tc + 16);
			}
			if (w <= width)
				ellipsisWidth = 0;		// enough room to draw entire string
			else {
				text = (const unsigned char *)ellipsis;		// draw ellipsis
				continue;
			}
		}

		sx = c_read16(cc + 4);
		sy = c_read16(cc + 6);
		sw = c_read16(cc + 8);
		sh = c_read16(cc + 10);
		cx = x + c_read16(cc + 12);
		cy = y + c_read16(cc + 14);

		if (isColor) {
			if (1 == isColor)
				PocoBitmapDraw(poco, &bits, cx, cy, sx, sy, sw, sh);
			else
				PocoBitmapDrawMasked(poco, kPocoOpaque, &bits, cx, cy, sx, sy, sw, sh,
					&mask, sx, sy);
		}
		else {
			if (isCompressed) {
				bits.pixels = (PocoPixel *)((sx | (sy << 16)) + (char *)cc);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
				bits.width = sw;
				bits.height = sh;
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
				bits.width = sh;
				bits.height = sw;
#endif
				PocoGrayBitmapDraw(poco, &bits, color, kPocoOpaque, cx, cy, 0, 0, sw, sh);
			}
			else
				PocoGrayBitmapDraw(poco, &bits, color, kPocoOpaque, cx, cy, sx, sy, sw, sh);
		}

		x += xadvance;
		width -= xadvance;
	}
}

void xs_poco_adaptInvalid(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	PixelsOutDispatch pixelsOutDispatch;

	if (!(kPocoFlagAdaptInvalid & poco->flags))
		return;

	pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;
	if (!pixelsOutDispatch)
		xsmcGet(xsResult, xsThis, xsID_pixelsOut);

#if 0 == kPocoRotation
	if (pixelsOutDispatch) {
		CommodettoRectangle cr = xsmcGetHostChunk(xsArg(0));
		(pixelsOutDispatch->doAdaptInvalid)(poco->outputRefcon, cr);
	}
	else
		xsCall1(xsResult, xsID_adaptInvalid, xsArg(0));
#else
	{
	CommodettoRectangle cr = xsmcGetHostChunk(xsArg(0));
	rotateCoordinatesAndDimensions(poco->width, poco->height, cr->x, cr->y, cr->w, cr->h);
	if (pixelsOutDispatch)
		(pixelsOutDispatch->doAdaptInvalid)(poco->outputRefcon, cr);
	else
		xsCall1(xsResult, xsID_adaptInvalidxsID_, xsArg(0));
	unrotateCoordinatesAndDimensions(poco->width, poco->height, cr->x, cr->y, cr->w, cr->h);
	}
#endif
}

void xs_poco_get_width(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	xsmcSetInteger(xsResult, poco->width);
}

void xs_poco_get_height(xsMachine *the)
{
	Poco poco = xsGetHostDataPoco(xsThis);
	xsmcSetInteger(xsResult, poco->height);
}

void xs_rectangle_get_x(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cr->x);
}

void xs_rectangle_get_y(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cr->y);
}

void xs_rectangle_get_w(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cr->w);
}

void xs_rectangle_get_h(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cr->h);
}

void xs_rectangle_set_x(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	cr->x = (PocoCoordinate)xsmcToInteger(xsArg(0));
}

void xs_rectangle_set_y(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	cr->y = (PocoCoordinate)xsmcToInteger(xsArg(0));
}

void xs_rectangle_set_w(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	cr->w = (PocoDimension)xsmcToInteger(xsArg(0));
}

void xs_rectangle_set_h(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);
	cr->h = (PocoDimension)xsmcToInteger(xsArg(0));
}

void xs_rectangle_set(xsMachine *the)
{
	CommodettoRectangle cr = xsmcGetHostChunk(xsThis);

	cr->x = (PocoCoordinate)xsmcToInteger(xsArg(0));
	cr->y = (PocoCoordinate)xsmcToInteger(xsArg(1));
	cr->w = (PocoDimension)xsmcToInteger(xsArg(2));
	cr->h = (PocoDimension)xsmcToInteger(xsArg(3));
}

void xs_rectangle_destructor(void *data)
{
}

void xs_rectangle_constructor(xsMachine *the)
{
	xsmcSetHostChunk(xsThis, NULL, sizeof(CommodettoRectangleRecord));
	xs_rectangle_set(the);
}
