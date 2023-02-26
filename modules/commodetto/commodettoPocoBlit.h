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


#ifndef __COMMODETTO_POCOBLIT_H__
#define __COMMODETTO_POCOBLIT_H__

#include <stdint.h>

#if MODINSTRUMENTATION
	#include "modInstrumentation.h"
	#define pocoInstrumentationSet modInstrumentationSet
	#define pocoInstrumentationMax modInstrumentationMax
	#define pocoInstrumentationMin modInstrumentationMin
	#define pocoInstrumentationAdjust modInstrumentationAdjust
	#define pocoInstrumentationGet modInstrumentationGet
#else
	#define pocoInstrumentationSet(what, value)
	#define pocoInstrumentationMax(what, value)
	#define pocoInstrumentationMin(what, value)
	#define pocoInstrumentationAdjust(what, value)
	#define pocoInstrumentationGet(what, value)
#endif

#include "commodettoBitmapFormat.h"
#include "commodettoBitmap.h"

#define kPocoPixelFormat kCommodettoBitmapFormat
#define kPocoPixelSize kCommodettoPixelSize

#ifndef kPocoFrameBuffer
	#define kPocoFrameBuffer 0
#endif

#ifndef kPocoRotation
	#define kPocoRotation 0
#endif

typedef enum {
	kPocoMonochromeForeground = 1,
	kPocoMonochromeBackground = 2,
	kPocoMonochromeForeAndBackground = (kPocoMonochromeForeground | kPocoMonochromeBackground)
} PocoMonochromeMode;

#define kPocoOpaque (255)

typedef CommodettoCoordinate PocoCoordinate;
typedef CommodettoDimension PocoDimension;
typedef CommodettoPixel PocoPixel;
typedef CommodettoBitmapFormat PocoBitmapFormat;

#if kCommodettoBitmapCLUT16 != kCommodettoBitmapFormat
	typedef CommodettoPixel PocoColor;
#else
	typedef uint16_t PocoColor;
#endif

typedef struct {
	PocoCoordinate	x;
	PocoCoordinate	y;
	PocoDimension	w;
	PocoDimension	h;
} PocoRectangleRecord, *PocoRectangle;

typedef struct PocoCommandRecord PocoCommandRecord;
typedef struct PocoCommandRecord *PocoCommand;

enum {
	kPocoFlagGCDisabled = 1 << 0,
	kPocoFlagDoubleBuffer = 1 << 1,
	kPocoFlagAdaptInvalid = 1 << 2,
	kPocoFlagFrameBuffer = 1 << 3,
	kPocoFlagErrorDisplayListOverflow = 1 << 4,
	kPocoFlagErrorStackProblem = 1 << 5,
	kPocoFlagDidBegin = 1 << 6,
	kPocoFlagContinue = 1 << 7,
	kPocoFlagBuffer = 1 << 8
};

struct PocoRecord {
	PocoDimension		width;			// with rotation applied (not physical)
	PocoDimension		height;			// with rotation applied (not physical)

	int					pixelsLength;

	char				*displayList;
	const char			*displayListEnd;
	PocoCommand			next;

	// clip rectangle of active drawing operation (rotation removed - physical)
	PocoCoordinate		x;
	PocoCoordinate		y;
	PocoDimension		w;
	PocoDimension		h;
	PocoCoordinate		xMax;
	PocoCoordinate		yMax;

	// origin
	PocoCoordinate		xOrigin;		// with rotation applied (not physical)
	PocoCoordinate		yOrigin;		// with rotation applied (not physical)

	int16_t				rowBytes;		// scanline stride for blitters

	uint16_t			flags;
	uint8_t				stackDepth;

	// native pixel output dispatch
	void				*outputRefcon;

#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	uint8_t				*clut;
#endif

#if kPocoFrameBuffer
	PocoPixel			*frameBuffer;
#endif

	PocoPixel			pixels[1];			// displayList follows pixels
};

typedef struct PocoRecord PocoRecord;
typedef struct PocoRecord *Poco;

typedef struct PocoBitmapRecord {
	PocoDimension		width;
	PocoDimension		height;
	PocoBitmapFormat	format;

	PocoPixel			*pixels;
#if COMMODETTO_BITMAP_ID
	uint32_t			id;
	int32_t				byteLength;
#endif
} PocoBitmapRecord, *PocoBitmap;

typedef void (*PocoRenderedPixelsReceiver)(PocoPixel *pixels, int byteCount, void *refCon);

#define PocoMakePixelGray256(r, g, b) (((r << 1) + r + (g << 2) + b) >> 3)
#define PocoMakePixelGray16(r, g, b) (((r << 1) + r + (g << 2) + b) >> 7)
#define PocoMakePixelRGB332(r, g, b) (((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6))
#define PocoMakePixelRGB565LE(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
#define PocoMakePixelCLUT16(r, g, b) (((r & 0xF0) << 4) | (g & 0xF0) | (b >> 4))

#if kCommodettoBitmapGray256 == kPocoPixelFormat
	#define PocoMakeColor(poco, r, g, b) PocoMakePixelGray256(r, g, b)
#elif kCommodettoBitmapGray16 == kPocoPixelFormat
	#define PocoMakeColor(poco, r, g, b) PocoMakePixelGray16(r, g, b)
#elif kCommodettoBitmapRGB332 == kPocoPixelFormat
	#define PocoMakeColor(poco, r, g, b) PocoMakePixelRGB332(r, g, b)
#elif kCommodettoBitmapRGB565LE == kPocoPixelFormat
	#define PocoMakeColor(poco, r, g, b) PocoMakePixelRGB565LE(r, g, b)
#elif kCommodettoBitmapCLUT16 == kPocoPixelFormat
	#define PocoMakeColor(poco, r, g, b) PocoMakePixelCLUT16(r, g, b)
#else
	#error
#endif

#ifdef __cplusplus
extern "C" {
#endif

void PocoDrawingBegin(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h);
int PocoDrawingEnd(Poco poco, PocoPixel *pixels, int byteLength, PocoRenderedPixelsReceiver pixelReceiver, void *refCon);

#if kPocoFrameBuffer
void PocoDrawingBeginFrameBuffer(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoPixel *pixels, int16_t rowBytes);
int PocoDrawingEndFrameBuffer(Poco poco);
#endif

void PocoRectangleFill(Poco poco, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h);
void PocoPixelDraw(Poco poco, PocoColor color, PocoCoordinate x, PocoCoordinate y);

void PocoBitmapDraw(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh);
void PocoMonochromeBitmapDraw(Poco poco, PocoBitmap bits, PocoMonochromeMode mode, PocoColor fgColor, PocoColor bgColor, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh);
void PocoGrayBitmapDraw(Poco poco, PocoBitmap bits, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh);

void PocoBitmapDrawMasked(Poco poco, uint8_t blend, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh,
			PocoBitmap mask, PocoDimension mask_sx, PocoDimension mask_sy);

void PocoBitmapPattern(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh);

void PocoDrawFrame(Poco poco, uint8_t *data, uint32_t dataSize, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h);

typedef void (*PocoRenderExternal)(Poco poco, uint8_t *data, PocoPixel *dst, PocoDimension w, PocoDimension h, uint8_t xphase);
void PocoDrawExternal(Poco poco, PocoRenderExternal doDrawExternal, uint8_t *data, uint8_t dataSize, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h);

int PocoClipPush(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h);		// 0 if clipped out
void PocoClipPop(Poco poco);

void PocoOriginPush(Poco poco, PocoCoordinate x, PocoCoordinate y);
void PocoOriginPop(Poco poco);

const uint8_t *PocoBMFGlyphFromUTF8(uint8_t **utf8Text, const uint8_t *bmfGlyphs, int glyphCount);
int PocoNextFromUTF8(uint8_t **src);

/*

	rotation of coordinates and dimensions
*/

#if 0 == kPocoRotation
	#define rotateCoordinatesAndDimensions(sw, sh, x, y, w, h)
	#define rotateCoordinates(sw, sh, x, y, w, h)
	#define rotateDimensions(w, h)
	#define unrotateCoordinatesAndDimensions(sw, sh, x, y, w, h)
	#define unrotateCoordinates(sw, sh, x, y, w, h)
	#define unrotateDimensions(w, h)
#elif 90 == kPocoRotation
	#define rotateCoordinatesAndDimensions(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x;					\
		PocoDimension td = w;					\
		x = sh - y - h;							\
		y = tc;									\
		w = h;									\
		h = td;									\
		}

	#define rotateCoordinates(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x;					\
		x = sh - y - h;							\
		y = tc;									\
		}

	#define rotateDimensions(w, h) { \
		PocoDimension td = w;					\
		w = h;									\
		h = td;									\
		}
	#define unrotateCoordinatesAndDimensions(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x; \
		PocoDimension td = w; \
		x = y; \
		y = sh - tc - w; \
		w = h; \
		h = td; \
	}
	#define urotateCoordinates(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x; \
		x = y; \
		y = sh - tc - w; \
	}
	#define unrotateDimensions(w, h) rotateDimensions(w, h)
#elif 270 == kPocoRotation
	#define rotateCoordinatesAndDimensions(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x;					\
		PocoDimension td = w;					\
		x = y;									\
		y = sw - tc - w;						\
		w = h;									\
		h = td;									\
		}

	#define rotateCoordinates(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x;					\
		x = y;									\
		y = sw - tc - w;						\
		}

	#define rotateDimensions(w, h) { \
		PocoDimension td = w;					\
		w = h;									\
		h = td;									\
		}
	#define unrotateCoordinatesAndDimensions(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x; \
		PocoDimension td = w; \
		x = sw - y - h; \
		y = tc; \
		w = h; \
		h = td; \
	}
	#define unrotateCoordinates(sw, sh, x, y, w, h) { \
		PocoCoordinate tc = x; \
		x = sw - y - h; \
		y = tc; \
	}
	#define unrotateDimensions(w, h) rotateDimensions(w, h)
#elif 180 == kPocoRotation
	#define rotateCoordinatesAndDimensions(sw, sh, x, y, w, h) \
		x = sw - x - w; \
		y = sh - y - h;

	#define rotateCoordinates(sw, sh, x, y, w, h) \
		x = sw - x - w; \
		y = sh - y - h;

	#define rotateDimensions(w, h)
	#define unrotateCoordinatesAndDimensions(sw, sh, x, y, w, h) rotateCoordinatesAndDimensions(sw, sh, x, y, w, h)
	#define unrotateCoordinates(sw, sh, x, y, w, h) rotateCoordinatesAndDimensions(sw, sh, x, y, w, h)
	#define unrotateDimensions(w, h)
#else
	#error must define kPocoRotation to 0, 90, 180, or 270
#endif

#define doPocoBlitterClip(x, y, w, h, xMax, yMax) \
	xMax = x + w;				\
	yMax = y + h; 				\
								\
	if (x < poco->x)			\
		x = poco->x;			\
								\
	if (xMax > poco->xMax)		\
		xMax = poco->xMax;		\
								\
	if (x >= xMax)				\
		return;					\
								\
	w = xMax - x;				\
								\
	if (y < poco->y)			\
		y = poco->y;			\
								\
	if (yMax > poco->yMax)		\
		yMax = poco->yMax;		\
								\
	if (y >= yMax)				\
		return;					\
								\
	h = yMax - y

#ifdef __cplusplus
}
#endif

#endif
