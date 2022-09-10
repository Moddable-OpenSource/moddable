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

#include "piuAll.h"
#include "mc.defines.h"
#include "commodettoPoco.h"
#include "commodettoFontEngine.h"

#ifdef piuGPU
	extern void PocoBitmapAdd(Poco poco, PocoBitmap bits, PocoRenderedPixelsReceiver pixelReceiver, void *refCon);
	extern void PocoBitmapChanged(Poco poco, uint16_t id, PocoRenderedPixelsReceiver pixelReceiver, void *refCon);
	extern void PocoBitmapRemove(Poco poco, uint16_t id, PocoRenderedPixelsReceiver pixelReceiver, void *refCon);
	extern void PocoCompact(Poco poco, PocoRenderedPixelsReceiver pixelReceiver, void *refCon);
	extern void PocoDrawImage(Poco poco, PocoBitmap bits, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h,
			PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh);
#endif

typedef struct PiuGlyphStruct PiuGlyphRecord, *PiuGlyph;
typedef struct PiuDieStruct PiuDieRecord, *PiuDie;
typedef struct PiuImageStruct PiuImageRecord, *PiuImage;
typedef struct PiuRegionStruct PiuRegionRecord, *PiuRegion;

// PiuFont.c

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIdentifier family;
	PiuCoordinate size;
	PiuCoordinate weight;
	uint8_t *buffer;
	size_t bufferSize;
	PiuDimension height;
	PiuDimension ascent;
	PiuTexture* texture;
};

#define PiuGlyphPart \
	uint16_t advance; \
	uint16_t sx; \
	uint16_t sy; \
	uint16_t sw; \
	uint16_t sh; \
	uint16_t dx; \
	uint16_t dy; \
	PocoBitmap bits; \
	PocoBitmap mask

struct PiuGlyphStruct {
	PiuGlyphPart;
};

extern PiuGlyph PiuFontGetGlyph(PiuFont* self, xsIntegerValue character, uint8_t needPixels);
extern void PiuFontListLockCache(xsMachine* the);
extern void PiuFontListUnlockCache(xsMachine* the);

extern CommodettoFontEngine gCFE;

// PiuTexture.c

enum {
	piuTextureAlpha = 1 << 0,
	piuTextureColor = 1 << 1,
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	PocoBitmapRecord bits;
	PocoBitmapRecord mask;
	PiuDimension width;
	PiuDimension height;
#ifdef piuGPU
	uint32_t usage;
#endif
};

#ifdef piuGPU
extern int piuTextureSize;
#endif

// PiuRegion.c

enum {
	piuRegionUnionOp = 0,
	piuRegionDifferenceOp = 1,
	piuRegionIntersectionOp = 3,
};

struct PiuRegionStruct {
	PiuHandlePart;
	PiuCoordinate available;
	PiuCoordinate data[11];
};

/*
	data
		length
		box
		[ span ]
	box
		x
		y
		width
		height
	span
		y
		offset to next span
		[ segment ]
	segment
		left
		right
*/

extern void PiuRegionNew(xsMachine* the, PiuCoordinate available);
// region0 = region1 op region2, return 1 if done, 0 if overflowed
extern PiuBoolean PiuRegionCombine(PiuRegion* region0, PiuRegion* region1, PiuRegion* region2, PiuCoordinate op);
// region0 = region1, return 1 if done, 0 if overflowed
extern PiuBoolean PiuRegionCopy(PiuRegion* region0, PiuRegion* region1);
// region = [ 5 0 0 0 0 ], return 1 if done, 0 if overflowed
extern PiuBoolean PiuRegionEmpty(PiuRegion* region);
// region = [ 11 x y w h y 2 x x+w y+h 0 ], return 1 if done, 0 if overflowed
extern PiuBoolean PiuRegionRectangle(PiuRegion* region, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h);
extern void PiuRegionOffset(PiuRegion* region, PiuCoordinate dx, PiuCoordinate dy);
extern PiuBoolean PiuRegionXOR(PiuRegion* region0, PiuRegion* region1, PiuRegion* region2);

// PiuDie.c

struct PiuDieStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
	PiuRegion* current;
	PiuRegion* swap;
	PiuRegion* work;
};

// PiuImage.c

struct PiuImageStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* archive;
	xsSlot* path;
	uint8_t *data;
	uint32_t dataSize;
	PiuDimension dataWidth;
	PiuDimension dataHeight;
	xsIntegerValue frameCount;
	xsIntegerValue frameIndex;
	uint32_t frameOffset;
	uint32_t frameSize;
#ifdef piuGPU
	uint32_t frameID;
	uint8_t frameChanged;
#endif
	uint8_t frameFormat;
};

// PiuView.c

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	Poco poco;
	PocoColor pixel;
	uint8_t blend;
	uint8_t updating;
	PiuTick idleTicks;
	PiuInterval idle;
	// cache references to accelerate the update loop
	xsSlot* screen;
	xsSlot* pixels;
	xsSlot* rectangle;
	xsSlot _adaptInvalid;
	xsSlot _begin;
	xsSlot _continue;
	xsSlot _end;
	xsSlot _send;
#ifdef piuGPU
	uint8_t dirty;
	uint8_t ready;
#else
	PiuRegion* dirty;
	PiuRegion* swap;
	uint32_t current;
	uint32_t limit;
	// commands...
#endif		
};


typedef void (*PiuViewDrawContentProc)(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);

extern void PiuViewDrawContent(PiuView* self, PiuViewDrawContentProc proc, void* it, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
extern void PiuViewDrawFrame(PiuView* self, uint8_t *data, uint32_t dataSize, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
extern void PiuViewInvalidateRegion(PiuView* self, PiuRegion* region);
extern void PiuViewReceiver(PocoPixel *pixels, int byteLength, void *refCon);
extern void PiuViewValidateRegion(PiuView* self, PiuRegion* region);
