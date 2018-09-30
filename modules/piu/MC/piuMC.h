/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "commodettoFontEngine.h"

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
	xsIndex family;
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
};

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
	xsSlot* path;
	uint8_t *data;
	uint32_t dataSize;
	PiuDimension dataWidth;
	PiuDimension dataHeight;
	xsIntegerValue frameCount;
	xsIntegerValue frameIndex;
	uint32_t frameOffset;
	uint32_t frameSize;
};

// PiuView.c

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	PiuRegion* dirty;
	PiuRegion* swap;
	uint32_t current;
	uint32_t limit;
	Poco poco;
	PocoColor pixel;
	uint8_t blend;
	// cache references to accelerate the update loop
	xsSlot* screen;
	xsSlot* pixels;
	xsSlot* rectangle;
	xsSlot _adaptInvalid;
	xsSlot _begin;
	xsSlot _continue;
	xsSlot _end;
	xsSlot _send;
	// commands...
};

extern void PiuViewDrawFrame(PiuView* self, uint8_t *data, uint32_t dataSize, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
extern void PiuViewInvalidateRegion(PiuView* self, PiuRegion* region);
extern void PiuViewValidateRegion(PiuView* self, PiuRegion* region);
