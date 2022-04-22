/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
 	validate source rectangles on all bitmaps (not just drawBitmap)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#if !XSTOOLS
	#include "mc.defines.h"
#endif

#include "commodettoPoco.h"
#include "commodettoPixelsOut.h"
#include "commodettoFontEngine.h"

#include "stddef.h"		// for offsetof macro
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"

#ifndef MODDEF_CFE_KERN
	#define MODDEF_CFE_KERN (0)
#endif

CommodettoFontEngine gCFE;

static PocoPixel *pocoGetBitmapPixels(xsMachine *the, Poco poco, CommodettoBitmap cb, int arg);

void xs_poco_destructor(void *data)
{
	if (data)
		c_free(((uint8_t *)data) - offsetof(PocoRecord, pixels));
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

	poco = c_malloc(sizeof(PocoRecord) + byteLength + 8);		// overhang when dividing
	if (!poco)
		xsErrorPrintf("no memory");
	xsmcSetHostBuffer(xsThis, poco->pixels, pixelsLength);

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
	xsmcDefine(xsThis, xsID_byteLength, xsVar(0), xsDefault);

	if (NULL == gCFE)
		gCFE = CFENew();
}

void xs_poco_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	if (data) {
		(void)xsmcGetHostDataPoco(xsThis);
		xs_poco_destructor(data);
		xsmcSetHostData(xsThis, NULL);
		xsSetHostDestructor(xsThis, NULL);
	}
}

void xs_poco_begin(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
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
#if MODDEF_DISPLAY_VERSION == 2
			*(int *)0 = 0;		// to do
#else
			xsmcVars(5);
			xsmcGet(xsVar(0), xsThis, xsID_pixelsOut);
			xsmcSetInteger(xsVar(1), poco->x);
			xsmcSetInteger(xsVar(2), poco->y);
			xsmcSetInteger(xsVar(3), poco->w);
			xsmcSetInteger(xsVar(4), poco->h);
			xsResult = xsCall4(xsVar(0), xsID_begin, xsVar(1), xsVar(2), xsVar(3), xsVar(4));
#endif
			pixels = xsmcGetHostBuffer(xsResult);

			xsmcSetInteger(xsResult, xsmcGetHostBufferLength(xsResult));
#if (0 == kPocoRotation) || (180 == kPocoRotation)
			rowBytes = (int16_t)(xsmcToInteger(xsVar(0)) / poco->height);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
			rowBytes = (int16_t)(xsmcToInteger(xsVar(0)) / poco->width);
#endif
		}

		PocoDrawingBeginFrameBuffer(poco, x, y, w, h, pixels, rowBytes);
	}
#endif

	CFELockCache(gCFE, true);

	poco->flags &= ~kPocoFlagGCDisabled;
}

static void pixelReceiver(PocoPixel *pixels, int byteLength, void *refCon)
{
	xsMachine *the = refCon;
	Poco poco = xsmcGetHostDataPoco(xsThis);		//@@ eliminate this call

	xsmcSetInteger(xsVar(1), (char *)pixels - (char *)poco->pixels);		// offset
	xsmcSetInteger(xsVar(2), (byteLength < 0) ? -byteLength : byteLength);
	xsCall3(xsVar(0), xsID_send, xsVar(3), xsVar(1), xsVar(2));
}

void xs_poco_end(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;

	if (!(poco->flags & kPocoFlagDidBegin))
		xsUnknownError("inactive");

	if (!(poco->flags & kPocoFlagFrameBuffer)) {
		int result;

		xsmcVars(5);

		if (pixelsOutDispatch) {
			(pixelsOutDispatch->doBegin)(poco->outputRefcon, poco->x, poco->y, poco->w, poco->h);
		}
		else {
			xsmcGet(xsVar(0), xsThis, xsID_pixelsOut);
#if MODDEF_DISPLAY_VERSION == 2
			xsmcSetNewObject(xsVar(1));
			xsmcSetInteger(xsVar(2), poco->x);
			xsmcSet(xsVar(1), xsID_x, xsVar(2));
			xsmcSetInteger(xsVar(2), poco->y);
			xsmcSet(xsVar(1), xsID_y, xsVar(2));
			xsmcSetInteger(xsVar(2), poco->w);
			xsmcSet(xsVar(1), xsID_width, xsVar(2));
			xsmcSetInteger(xsVar(2), poco->h);
			xsmcSet(xsVar(1), xsID_height, xsVar(2));
			if (poco->flags & kPocoFlagContinue) {
				xsmcSetTrue(xsVar(2));
				xsmcSet(xsVar(1), xsID_continue, xsVar(2));
			}
			xsCall1(xsVar(0), xsID_begin, xsVar(1));
#else
			xsmcSetInteger(xsVar(1), poco->x);
			xsmcSetInteger(xsVar(2), poco->y);
			xsmcSetInteger(xsVar(3), poco->w);
			xsmcSetInteger(xsVar(4), poco->h);
			xsCall4(xsVar(0), xsID_begin, xsVar(1), xsVar(2), xsVar(3), xsVar(4));
#endif
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
#if MODDEF_DISPLAY_VERSION == 2
		poco->flags |= kPocoFlagContinue;
#else
		if (pixelsOutDispatch)
			(pixelsOutDispatch->doContinue)(poco->outputRefcon);
		else
			xsCall0(xsVar(0), xsID_continue);
#endif
	}
	else {
#if MODDEF_DISPLAY_VERSION == 2
		poco->flags &= ~kPocoFlagContinue;
#endif
		if (pixelsOutDispatch)
			(pixelsOutDispatch->doEnd)(poco->outputRefcon);
		else
			xsCall0(xsVar(0), xsID_end);

		pocoInstrumentationAdjust(FramesDrawn, 1);
	}

	CFELockCache(gCFE, false);
}

void xs_poco_makeColor(xsMachine *the)
{
	int r, g, b, color;

	r = xsmcToInteger(xsArg(0));
	g = xsmcToInteger(xsArg(1));
	b = xsmcToInteger(xsArg(2));

	color = PocoMakeColor(xsmcGetHostDataPoco(xsThis), r, g, b);

	xsmcSetInteger(xsResult, color);
}

void xs_poco_clip(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;

	if (argc) {
		PocoCoordinate x, y;
		PocoDimension w, h;

		x = (PocoCoordinate)xsmcToInteger(xsArg(0)) + poco->xOrigin;
		y = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->yOrigin;
		w = (PocoDimension)xsmcToInteger(xsArg(2));
		h = (PocoDimension)xsmcToInteger(xsArg(3));
		if (PocoClipPush(poco, x, y, w, h))
			xsmcSetTrue(xsResult);
	}
	else
		PocoClipPop(poco);
}

void xs_poco_origin(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
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
	Poco poco = xsmcGetHostDataPoco(xsThis);
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
	Poco poco = xsmcGetHostDataPoco(xsThis);
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
	Poco poco = xsmcGetHostDataPoco(xsThis);
	PocoColor color;
	PocoCoordinate x, y;

	color = (PocoColor)xsmcToInteger(xsArg(0));
	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;

	PocoPixelDraw(poco, color, x, y);
}

void xs_poco_drawBitmap(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	CommodettoBitmap cb;

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
	bits.id = cb->id;
	bits.byteLength = (cb->flags & kCommodettoBitmapHaveByteLength) ? cb->byteLength : 0;
#endif
	bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;

	if (argc <= 3) {
		sx = 0, sy = 0;
#if (90 == kPocoRotation) || (270 == kPocoRotation)
		sw = bits.height, sh = bits.width;
#else
		sw = bits.width, sh = bits.height;
#endif
	}
	else {
		sx = (PocoDimension)xsmcToInteger(xsArg(3));
		sy = (PocoDimension)xsmcToInteger(xsArg(4));
		sw = (PocoDimension)xsmcToInteger(xsArg(5));
		sh = (PocoDimension)xsmcToInteger(xsArg(6));
#if (0 == kPocoRotation) || (180 == kPocoRotation)
		if ((sx >= bits.width) || (sy >= bits.height) || ((sx + sw) > bits.width) || ((sy + sh) > bits.height))
#else
		if ((sx >= bits.height) || (sy >= bits.width) || ((sx + sw) > bits.height) || ((sy + sh) > bits.width))
#endif
			xsRangeError("invalid src");
	}

	PocoBitmapDraw(poco, &bits, x, y, sx, sy, sw, sh);
}

void xs_poco_drawMonochrome(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	PocoColor fgColor = 0, bgColor = 0;
	PocoMonochromeMode mode = 0;
	CommodettoBitmap cb;

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
	bits.id = cb->id;
	bits.byteLength = 0;
#endif
	bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

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

	if (argc == 5) {
		sx = 0, sy = 0;
#if (90 == kPocoRotation) || (270 == kPocoRotation)
		sw = bits.height, sh = bits.width;
#else
		sw = bits.width, sh = bits.height;
#endif
	}
	else {
		sx = (PocoDimension)xsmcToInteger(xsArg(5));
		sy = (PocoDimension)xsmcToInteger(xsArg(6));
		sw = (PocoDimension)xsmcToInteger(xsArg(7));
		sh = (PocoDimension)xsmcToInteger(xsArg(8));

#if (0 == kPocoRotation) || (180 == kPocoRotation)
		if ((sx >= bits.width) || (sy >= bits.height) || ((sx + sw) > bits.width) || ((sy + sh) > bits.height))
#else
		if ((sx >= bits.height) || (sy >= bits.width) || ((sx + sw) > bits.height) || ((sy + sh) > bits.width))
#endif
			xsRangeError("invalid src");
	}

	PocoMonochromeBitmapDraw(poco, &bits, mode, fgColor, bgColor, x, y, sx, sy, sw, sh);
}

void xs_poco_drawGray(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	PocoColor color = 0;
	CommodettoBitmap cb;
	uint8_t blend = kPocoOpaque;

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
	bits.id = cb->id;
	bits.byteLength = 0;
#endif
	bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

	color = (PocoColor)xsmcToInteger(xsArg(1));

	x = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->yOrigin;

	if (argc <= 4) {
		sx = 0, sy = 0;
	#if (90 == kPocoRotation) || (270 == kPocoRotation)
		sw = bits.height, sh = bits.width;
	#else
		sw = bits.width, sh = bits.height;
	#endif
	}
	else {
		sx = (PocoDimension)xsmcToInteger(xsArg(4));
		sy = (PocoDimension)xsmcToInteger(xsArg(5));
		sw = (PocoDimension)xsmcToInteger(xsArg(6));
		sh = (PocoDimension)xsmcToInteger(xsArg(7));

#if (0 == kPocoRotation) || (180 == kPocoRotation)
		if ((sx >= bits.width) || (sy >= bits.height) || ((sx + sw) > bits.width) || ((sy + sh) > bits.height))
#else
		if ((sx >= bits.height) || (sy >= bits.width) || ((sx + sw) > bits.height) || ((sy + sh) > bits.width))
#endif
			xsRangeError("invalid src");

		if (argc > 8)
			blend = (uint8_t)xsmcToInteger(xsArg(8));
	}

	PocoGrayBitmapDraw(poco, &bits, color, blend, x, y, sx, sy, sw, sh);
}

void xs_poco_drawMasked(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	PocoBitmapRecord bits, mask;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh, mask_sx, mask_sy;
	CommodettoBitmap cb;

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
	bits.id = cb->id;
	bits.byteLength = 0;
#endif
	bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	sx = (PocoDimension)xsmcToInteger(xsArg(3));
	sy = (PocoDimension)xsmcToInteger(xsArg(4));
	sw = (PocoDimension)xsmcToInteger(xsArg(5));
	sh = (PocoDimension)xsmcToInteger(xsArg(6));

#if (0 == kPocoRotation) || (180 == kPocoRotation)
	if ((sx >= bits.width) || (sy >= bits.height) || ((sx + sw) > bits.width) || ((sy + sh) > bits.height))
#else
	if ((sx >= bits.height) || (sy >= bits.width) || ((sx + sw) > bits.height) || ((sy + sh) > bits.width))
#endif
		xsRangeError("invalid src");

	cb = xsmcGetHostChunk(xsArg(7));
	mask.width = cb->w;
	mask.height = cb->h;
	mask.format = cb->format;
#if COMMODETTO_BITMAP_ID
	mask.id = cb->id;
	mask.byteLength = 0;
#endif
	mask.pixels = pocoGetBitmapPixels(the, poco, cb, 7);

	mask_sx = (PocoDimension)xsmcToInteger(xsArg(8));
	mask_sy = (PocoDimension)xsmcToInteger(xsArg(9));
	if ((mask_sx >= mask.width) || (mask_sy >= mask.height) || ((mask_sx + sw) > mask.width) || ((mask_sy + sh) > mask.height))
		xsRangeError("invalid mask src");

	if (argc > 10) {
		uint8_t blend = (uint8_t)xsmcToInteger(xsArg(10));
		PocoBitmapDrawMasked(poco, blend, &bits, x, y, sx, sy, sw, sh, &mask, mask_sx, mask_sy);
	}
	else
		PocoBitmapDrawMasked(poco, kPocoOpaque, &bits, x, y, sx, sy, sw, sh, &mask, mask_sx, mask_sy);
}

void xs_poco_fillPattern(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	PocoBitmapRecord bits;
	PocoCoordinate x, y;
	PocoDimension w, h, sx, sy, sw, sh;
	CommodettoBitmap cb;
	int argc = xsmcArgc;

	cb = xsmcGetHostChunk(xsArg(0));
	bits.width = cb->w;
	bits.height = cb->h;
	bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
	bits.id = cb->id;
	bits.byteLength = 0;
#endif
	bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	w = (PocoDimension)xsmcToInteger(xsArg(3));
	h = (PocoDimension)xsmcToInteger(xsArg(4));
	if (argc > 5) {
		sx = (PocoCoordinate)xsmcToInteger(xsArg(5));
		sy = (PocoCoordinate)xsmcToInteger(xsArg(6));
		sw = (PocoDimension)xsmcToInteger(xsArg(7));
		sh = (PocoDimension)xsmcToInteger(xsArg(8));

#if (0 == kPocoRotation) || (180 == kPocoRotation)
		if ((sx >= bits.width) || (sy >= bits.height) || ((sx + sw) > bits.width) || ((sy + sh) > bits.height))
#else
		if ((sx >= bits.height) || (sy >= bits.width) || ((sx + sw) > bits.height) || ((sy + sh) > bits.width))
#endif
			xsRangeError("invalid src");

		PocoBitmapPattern(poco, &bits, x, y, w, h, sx, sy, sw, sh);
	}
	else
		PocoBitmapPattern(poco, &bits, x, y, w, h, 0, 0, bits.width, bits.height);
}

void xs_poco_drawFrame(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	uint8_t *data;
	xsUnsignedValue dataSize;
	PocoCoordinate x, y;
	PocoDimension w, h;

	x = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->yOrigin;

	xsmcGet(xsResult, xsArg(1), xsID_width);
	w = xsmcToInteger(xsResult);
	xsmcGet(xsResult, xsArg(1), xsID_height);
	h = xsmcToInteger(xsResult);

	if (xsBufferRelocatable == xsmcGetBufferReadable(xsArg(0), (void **)&data, &dataSize))
		PocoDisableGC(poco);

	PocoDrawFrame(poco, data, dataSize, x, y, w, h);
}

void xs_poco_getTextWidth(xsMachine *the)
{
	const unsigned char *text = (const unsigned char *)xsmcToString(xsArg(0));
	const char *fontData;
	int width = 0;
#if MODDEF_CFE_KERN
	uint32_t previousUnicode = 0;
#endif

	fontData = xsmcGetHostData(xsArg(1));
	CFESetFontData(gCFE, fontData, xsmcGetHostBufferLength(xsArg(1)));

	while (true) {
		CFEGlyph glyph;
		uint32_t unicode = PocoNextFromUTF8((uint8_t **)&text);
		if (!unicode) {
			if (!c_read8(text - 1))
				break;
			continue;
		}

		glyph = CFEGetGlyphFromUnicode(gCFE, unicode, false);
		if (glyph) {
			width += glyph->advance;
#if MODDEF_CFE_KERN
			if (previousUnicode)
				width += CFEGetKerningOffset(gCFE, previousUnicode, unicode);
			previousUnicode = unicode;
#endif
		}
	}

	xsmcSetInteger(xsResult, width);
}

void xs_poco_drawText(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	int argc = xsmcArgc;
	const unsigned char *text = (const unsigned char *)xsmcToString(xsArg(0));
	PocoCoordinate x, y;
	PocoColor color;
	CommodettoBitmap cb = NULL;
	PocoBitmapRecord bits;
	static const unsigned char *ellipsisFallback = (unsigned char *)"...";
	static const unsigned char ellipsisUTF8[4] = {0xE2, 0x80, 0xA6, 0};		// 0x2026
	const unsigned char *ellipsis;
	PocoDimension ellipsisWidth;
	int width;
	const unsigned char *fontData;
	uint8_t isColor;
	PocoBitmapRecord mask;
#if MODDEF_CFE_KERN
	uint32_t previousUnicode = 0;
#endif

	x = (PocoCoordinate)xsmcToInteger(xsArg(3)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(4)) + poco->yOrigin;

	xsmcVars(2);

	fontData = xsmcGetHostData(xsArg(1));
	CFESetFontData(gCFE, fontData, xsmcGetHostBufferLength(xsArg(1)));

	if (argc > 5) {
		CFEGlyph glyph = CFEGetGlyphFromUnicode(gCFE, 0x2026, false);
		if (glyph) {
			ellipsisWidth = glyph->advance;
			ellipsis = ellipsisUTF8;
		}
		else {
			glyph = CFEGetGlyphFromUnicode(gCFE, '.', false);
			ellipsisWidth = glyph ? glyph->advance * 3 : 0;
			ellipsis = ellipsisFallback;
		}

		width = xsmcToInteger(xsArg(5));
	}
	else {
		width = 0;
		ellipsisWidth = 0;
	}

	while (true) {
		PocoCoordinate cx, cy, sx, sy;
		PocoDimension sw, sh;
		CFEGlyph glyph;
		uint32_t unicode = PocoNextFromUTF8((uint8_t **)&text);
		if (!unicode) {
			if (!c_read8(text - 1))
				break;
			continue;
		}

		glyph = CFEGetGlyphFromUnicode(gCFE, unicode, true);
		if (NULL == glyph)
			continue;

#if MODDEF_CFE_KERN
		if (previousUnicode) {
			int16_t kerningOffset = CFEGetKerningOffset(gCFE, previousUnicode, unicode);
			width += kerningOffset;
			x += kerningOffset;
		}
		previousUnicode = unicode;
#endif

		if (ellipsisWidth && ((width - glyph->advance) < ellipsisWidth)) {
			// measure the rest of the string to see if it fits
			const unsigned char *t = text;
			int w = glyph->advance;
			while (w <= width) {
				CFEGlyph tg;
				uint32_t unicode = PocoNextFromUTF8((uint8_t **)&t);
				if (!unicode) {
					if (!c_read8(t - 1))
						break;
					continue;
				}
				tg = CFEGetGlyphFromUnicode(gCFE, unicode, false);
				if (!tg) continue;

				w += tg->advance;
			}
			if (w > width) {
				if ((text == (ellipsis + 2)) || (text == (ellipsis + 3))) {
					width = ellipsisWidth;						// not enough space for ellipsis. draw one dot.
					text = ellipsis + 2;
				}
				else if (text == (ellipsis + 1))
					;		// try again with fewer characters
				else
					text = (const unsigned char *)ellipsis;		// draw ellipsis
				continue;
			}
			ellipsisWidth = 0;		// enough room to draw entire string
			glyph = CFEGetGlyphFromUnicode(gCFE, unicode, true);
		}

		sx = glyph->sx;
		sy = glyph->sy;
		sw = glyph->w;
		sh = glyph->h;
		cx = x + glyph->dx;
		cy = y + glyph->dy;

		if (glyph->format) {
			if (NULL == cb) {
				color = (PocoColor)xsmcToInteger(xsArg(2));
				cb = (void *)1;
			}
#if (0 == kPocoRotation) || (180 == kPocoRotation)
			bits.width = glyph->w;
			bits.height = glyph->h;
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
			bits.width = glyph->h;
			bits.height = glyph->w;
#endif
			bits.format = glyph->format;
			bits.pixels = (PocoPixel *)glyph->bits;
#if COMMODETTO_BITMAP_ID
			bits.id = glyph->id;		//@@ hack that barely works for a single font..
			bits.byteLength = 0;
#endif

			PocoGrayBitmapDraw(poco, &bits, color, kPocoOpaque, cx, cy, sx, sy, sw, sh);
		}
		else {
			if (NULL == cb) {
				// fill out the bitmap
				xsmcGet(xsVar(0), xsArg(1), xsID_bitmap);
				cb = xsmcGetHostChunk(xsVar(0));
				bits.width = cb->w;
				bits.height = cb->h;
				bits.format = cb->format;
#if COMMODETTO_BITMAP_ID
				bits.id = cb->id;
				bits.byteLength = 0;
#endif
				bits.pixels = pocoGetBitmapPixels(the, poco, cb, 0);

				if (kPocoPixelFormat == cb->format) {
					isColor = 1;
					if (xsReferenceType == xsmcTypeOf(xsArg(2))) {
						isColor = 2;

						cb = xsmcGetHostChunk(xsArg(2));
						mask.width = cb->w;
						mask.height = cb->h;
						mask.format = cb->format;
#if COMMODETTO_BITMAP_ID
						mask.id = cb->id;
						mask.byteLength = 0;
#endif
						mask.pixels = pocoGetBitmapPixels(the, poco, cb, 2);
					}
				}
				else {
					isColor = 0;
					color = (PocoColor)xsmcToInteger(xsArg(2));
				}
			}

			if (isColor) {
				if (1 == isColor)
					PocoBitmapDraw(poco, &bits, cx, cy, sx, sy, sw, sh);
				else
					PocoBitmapDrawMasked(poco, kPocoOpaque, &bits, cx, cy, sx, sy, sw, sh,
						&mask, sx, sy);
			}
			else
				PocoGrayBitmapDraw(poco, &bits, color, kPocoOpaque, cx, cy, sx, sy, sw, sh);
		}

		x += glyph->advance;
		width -= glyph->advance;
	}
}

void xs_poco_adaptInvalid(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
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
		xsCall1(xsResult, xsID_adaptInvalid, xsArg(0));
	unrotateCoordinatesAndDimensions(poco->width, poco->height, cr->x, cr->y, cr->w, cr->h);
	}
#endif
}

void xs_poco_get_width(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	xsmcSetInteger(xsResult, poco->width);
}

void xs_poco_get_height(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	xsmcSetInteger(xsResult, poco->height);
}

PocoPixel *pocoGetBitmapPixels(xsMachine *the, Poco poco, CommodettoBitmap cb, int arg)
{
	xsSlot buffer;
	void *data;
	xsUnsignedValue dataSize;
	int32_t offset;

	if (cb->havePointer)
		return (PocoPixel *)cb->bits.data;
	
	offset = cb->bits.offset;
	xsmcGet(buffer, xsArg(arg), xsID_buffer);
	xsmcGetBufferReadable(buffer, &data, &dataSize);
	PocoDisableGC(poco);
	return (PocoPixel *)(offset + (char *)data);
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
