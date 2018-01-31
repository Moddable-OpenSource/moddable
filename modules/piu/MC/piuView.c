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

#include "piuAll.h"
#include "piuMC.h"

enum {
	piuDrawFrameCommand,
	piuDrawStringCommand,
	piuDrawTextureCommand,
	piuFillColorCommand,
	piuFillTextureCommand,
	piuPopClipCommand,
	piuPushClipCommand,
	piuEndCommand,
};

typedef uint8_t PiuCommandID;

typedef struct {
	PiuCommandID id;
	uint8_t *data;
	uint32_t dataSize;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoDimension sw;
	PocoDimension sh;
} PiuDrawFrameCommand;

typedef struct {
	PiuCommandID id;
	uint8_t blend;
	PocoColor color;
	xsIntegerValue offset;
	xsIntegerValue length;
	xsSlot* string;
	PiuFont* font;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoDimension w;
	PocoDimension sw;
} PiuDrawStringCommand;

typedef struct {
	PiuCommandID id;
	uint8_t blend;
	PocoColor color;
	PiuTexture* texture;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoCoordinate sx;
	PocoCoordinate sy;
	PocoDimension sw;
	PocoDimension sh;
} PiuDrawTextureCommand;

typedef struct {
	PiuCommandID id;
	uint8_t blend;
	PocoColor color;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoDimension w;
	PocoDimension h;
} PiuFillColorCommand;

typedef struct {
	PiuCommandID id;
	uint8_t blend;
	PocoColor color;
	PiuTexture* texture;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoDimension w;
	PocoDimension h;
	PocoCoordinate sx;
	PocoCoordinate sy;
	PocoDimension sw;
	PocoDimension sh;
} PiuFillTextureCommand;

typedef struct {
	PiuCommandID id;
	uint16_t	pad;
} PiuPopClipCommand;

typedef struct {
	PiuCommandID id;
	PocoCoordinate x;
	PocoCoordinate y;
	PocoDimension w;
	PocoDimension h;
	uint16_t	pad;
} PiuPushClipCommand;

typedef struct {
	PiuCommandID id;
	uint16_t	pad;
} PiuEndCommand;

static void PiuViewBegin(PiuView* view);
static void PiuViewCombine(PiuView* self, PiuRectangle area, PiuCoordinate op);
static void PiuViewCombineRegion(PiuView* self, PiuRegion* region, PiuCoordinate op);
static void PiuViewDrawStringAux(PiuView* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw);
static void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewEnd(PiuView* view);
static void PiuViewFillTextureAux(PiuView* self, PiuTexture* texture, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuViewReceiver(PocoPixel *pixels, int byteLength, void *refCon);
static void PiuViewUpdate(PiuView* self, PiuApplication* application);
static void PiuViewUpdateStep(PiuView* self, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, uint8_t flag); 

static xsHostHooks PiuViewHooks ICACHE_RODATA_ATTR = {
	PiuViewDelete,
	PiuViewMark,
	NULL
};

#define PIUQueueCommand(_COMMAND) \
	Piu##_COMMAND* command = (Piu##_COMMAND*)(((uint8_t*)(*self)) + (*self)->current); \
	if ((*self)->limit - (*self)->current < sizeof(Piu##_COMMAND)) { \
		xsMachine* the = (*self)->the; \
		xsErrorPrintf("command list overflowed"); \
	} \
	(*self)->current += sizeof(Piu##_COMMAND); \
	command->id = piu##_COMMAND

#if (defined(__GNUC__) && defined(__OPTIMIZE__)) || defined(__llvm__)
	#define PIUBreak \
		goto *dispatches[*((PiuCommandID*)(((uint8_t*)(*self)) + current))]; \
		} continue
	#define PIUCase(_COMMAND) \
		PIU##_COMMAND: { \
		Piu##_COMMAND* command = (Piu##_COMMAND*)(((uint8_t*)(*self)) + current); \
		current += sizeof(Piu##_COMMAND);
	#define PIUSwitch(ID) goto *dispatches[ID];
#else
	#define PIUBreak \
		} break
	#define PIUCase(_COMMAND) \
		case piu##_COMMAND: { \
			Piu##_COMMAND* command = (Piu##_COMMAND*)(((uint8_t*)(*self)) + current); \
			current += sizeof(Piu##_COMMAND);
	#define PIUSwitch(ID) \
		switch(ID)
#endif

#define xsGetHostDataPoco(slot) ((void *)((char *)xsGetHostData(slot) - offsetof(PocoRecord, pixels)))

void PiuViewAdjust(PiuView* self) 
{
}

void PiuViewBegin(PiuView* self) 
{
	Poco poco = (*self)->poco;
	PocoCoordinate x, y;
	PocoDimension w, h;
	
	x = 0;
	y = 0;
	w = poco->width;
	h = poco->height;
	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
	
	poco->flags &= ~(kPocoFlagErrorDisplayListOverflow | kPocoFlagErrorStackProblem | kPocoFlagGCDisabled);
	poco->stackDepth = 0;
	poco->xOrigin = poco->yOrigin = 0;

	poco->x = x;
	poco->y = y;
	poco->w = w;
	poco->h = h;

	poco->xMax = x + w;
	poco->yMax = y + h;

#if kPocoFrameBuffer
	poco->frameBuffer = NULL;
#endif

	(*self)->current = sizeof(PiuViewRecord);
}

void PiuViewCombine(PiuView* self, PiuRectangle area, PiuCoordinate op) 
{
	xsMachine* the = (*self)->the;
	Poco poco = (*self)->poco;
	PiuRegion* tmp = (*self)->dirty;
	(*self)->dirty = (*self)->swap;
	(*self)->swap = tmp;
	if (area) {
		PiuRegionRecord regionRecord;
		PiuRegion region = &regionRecord;
		PocoCoordinate left = area->x;
		PocoCoordinate top = area->y;
		PocoCoordinate right = left + area->width;
		PocoCoordinate bottom = top + area->height;
		PocoCoordinate x, y;
		PocoDimension w, h;
		if (left < 0) left = 0;
		else if (left > poco->width) left = poco->width;
		if (top < 0) top = 0;
		else if (top > poco->height) top = poco->height;
		if (right < 0) right = 0;
		else if (right > poco->width) right = poco->width;
		if (bottom < 0) bottom = 0;
		else if (bottom > poco->height) bottom = poco->height;
		x = left;
		y = top;
		w = right - left;
		h = bottom - top;
		if (poco->flags & kPocoFlagAdaptInvalid) {
			PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;
			CommodettoRectangle cr = xsGetHostChunk(xsReference((*self)->rectangle));
			rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
			cr->x = x;
			cr->y = y;
			cr->w = w;
			cr->h = h;
			if (pixelsOutDispatch)
				(pixelsOutDispatch->doAdaptInvalid)(poco->outputRefcon, cr);
			else
				xsCallFunction1((*self)->_adaptInvalid, xsReference((*self)->screen), xsReference((*self)->rectangle));
			x = cr->x;
			y = cr->y;
			w = cr->w; 
			h = cr->h; 
			unrotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
		}
		regionRecord.reference = NULL;
		regionRecord.available = 11;
		PiuRegionRectangle(&region, x, y, w, h);
		if (PiuRegionCombine((*self)->dirty, (*self)->swap, &region, op)) {
			return;
		}
	}
	else if (op == piuRegionUnionOp) {
		if (PiuRegionRectangle((*self)->dirty, 0, 0, poco->width, poco->height))
			return;
	}
	else {
		if (PiuRegionEmpty((*self)->dirty))
			return;
	}
	xsErrorPrintf("dirty region overflowed");
}

void PiuViewCombineRegion(PiuView* self, PiuRegion* region, PiuCoordinate op) 
{
	xsMachine* the = (*self)->the;
	Poco poco = (*self)->poco;
	PiuRegion* tmp;
	if (poco->flags & kPocoFlagAdaptInvalid) {
		PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;
		CommodettoRectangle cr = xsGetHostChunk(xsReference((*self)->rectangle));
		PocoCoordinate *regionBegin0 = (*region)->data;
		int offset = 5;
		int size = *regionBegin0 - 2; // last span
		PocoCoordinate *region0 = regionBegin0 + offset;
		PocoCoordinate *regionLimit0 = regionBegin0 + size;
		PiuRegionRecord rectangleRegionRecord;
		PiuRegion rectangleRegion = &rectangleRegionRecord;
		rectangleRegionRecord.reference = NULL;
		rectangleRegionRecord.available = 11;
		while (region0 < regionLimit0) {
			PocoCoordinate top = *region0++;
			PocoCoordinate segmentCount = *region0++;
			PocoCoordinate bottom = *(region0 + segmentCount);		
			while (segmentCount) {
				PocoCoordinate left = *region0++;
				PocoCoordinate right = *region0++;
				PocoCoordinate x, y;
				PocoDimension w, h;
				segmentCount -= 2;
				offset = region0 - regionBegin0;
				x = left;
				y = top;
				w = right - left;
				h = bottom - top;
				rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
				cr->x = x;
				cr->y = y;
				cr->w = w;
				cr->h = h;
				if (pixelsOutDispatch)
					(pixelsOutDispatch->doAdaptInvalid)(poco->outputRefcon, cr);
				else
					xsCallFunction1((*self)->_adaptInvalid, xsReference((*self)->screen), xsReference((*self)->rectangle));
				x = cr->x;
				y = cr->y;
				w = cr->w; 
				h = cr->h; 
				unrotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
				PiuRegionRectangle(&rectangleRegion, x, y, w, h);
				tmp = (*self)->dirty;
				(*self)->dirty = (*self)->swap;
				(*self)->swap = tmp;
				if (!PiuRegionCombine((*self)->dirty, (*self)->swap, &rectangleRegion, op)) {
					xsErrorPrintf("dirty region overflowed");
				}
				regionBegin0 = (*region)->data;
				region0 = regionBegin0 + offset;
				regionLimit0 = regionBegin0 + size;
			}
		}
	}
	else {
		tmp = (*self)->dirty;
		(*self)->dirty = (*self)->swap;
		(*self)->swap = tmp;
		if (!PiuRegionCombine((*self)->dirty, (*self)->swap, region, op)) {
			xsErrorPrintf("dirty region overflowed");
		}
	}
}

void PiuViewDelete(void* it)
{
}

void PiuViewDictionary(xsMachine* the, void* it)
{
	PiuApplication* self = it;
	PiuView* view = (*self)->view;
	PiuDimension width, height;
	PiuViewGetSize(view, &width, &height);
	(*self)->coordinates.horizontal = piuWidth;
	(*self)->coordinates.left = (*self)->coordinates.right = (*self)->bounds.x = 0;
	(*self)->coordinates.width = (*self)->bounds.width = width;
	(*self)->coordinates.vertical = piuHeight;
	(*self)->coordinates.top = (*self)->coordinates.bottom = (*self)->bounds.y = 0;
	(*self)->coordinates.height = (*self)->bounds.height = height;
}

void PiuViewDrawFrame(PiuView* self, uint8_t *data, uint32_t dataSize, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	Poco poco = (*self)->poco;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(DrawFrameCommand);
		command->data = data;
		command->dataSize = dataSize;
		command->x = x;
		command->y = y;
		command->sw = sw;
		command->sh = sh;
	}
}

void PiuViewDrawString(PiuView* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw)
{
	Poco poco = (*self)->poco;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(DrawStringCommand);
		command->string = string;
		command->offset = offset;
		command->length = length;
		command->font = font;
		command->color = (*self)->pixel;
		command->blend = (*self)->blend;
		command->x = x;
		command->y = y;
		command->w = w;
		command->sw = sw;
	}
}

void PiuViewDrawStringAux(PiuView* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuDimension width, PiuDimension stringWidth)
{
	xsMachine* the = (*self)->the;
	Poco poco = (*self)->poco;
	xsStringValue text = PiuToString(string);
	const unsigned char *chars = (*font)->buffer + (*font)->offset;
	uint32_t charCount;
	PiuTexture* texture = (*font)->texture;
	PocoBitmap bits, mask;
	PocoBitmapRecord pack;
	
	static const char *ellipsis = "...";
	PocoDimension ellipsisWidth;
	
	charCount = c_read32(chars) / 20;
	chars += 4;

	if (width) {
		const char *t = ellipsis;
		const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t **)&t, chars, charCount);

		if (cc) {
			ellipsisWidth = c_read16(cc + 16);						// +16 -> offset to xadvance
			ellipsisWidth *= 3;
		}
		else
			ellipsisWidth = 0;
	}
	else
		ellipsisWidth = 0;

	if (texture) {
		PiuFlags flags = (*texture)->flags;
		bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
		mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
	}
	else {
		bits = NULL;
		mask = NULL;
		pack.format = kCommodettoBitmapGray16 | kCommodettoBitmapPacked;
	}

	text += offset;
	while (length) {
		PocoCoordinate cx, cy, sx, sy;
		PocoDimension sw, sh;
		uint16_t xadvance;
		char *prev = text;
		const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t **)&text, chars, charCount);

		length -= (PiuDimension)(text - prev);
		if (!cc) {
			uint8_t missing, *pp;

			if (!c_read8(prev))
				break;

			missing = '?';
			pp = &missing;
			cc = PocoBMFGlyphFromUTF8(&pp, chars, charCount);
			if (NULL == cc)
				continue;
		}

		xadvance = c_read16(cc + 16);	// +16 -> offset to xadvance

		if (ellipsisWidth && ((width - xadvance) <= ellipsisWidth)) {
			ellipsisWidth = 0;
			if (stringWidth > width) {
				text = (xsStringValue)ellipsis;
				length = 3;
				continue;
			}
		}
			
		cx = x + c_read16(cc + 12);
		cy = y + c_read16(cc + 14);
		sx = c_read16(cc + 4);
		sy = c_read16(cc + 6);
		sw = c_read16(cc + 8);
		sh = c_read16(cc + 10);

		if (mask) {
			if (bits)
				PocoBitmapDrawMasked(poco, blend, bits, cx, cy, sx, sy, sw, sh, mask, sx, sy);
			else
				PocoGrayBitmapDraw(poco, mask, color, blend, cx, cy, sx, sy, sw, sh);
		}
		else {
			if (bits)
				PocoBitmapDraw(poco, bits, cx, cy, sx, sy, sw, sh);
			else {
				pack.pixels = (PocoPixel *)((sx | (sy << 16)) + (char *)cc);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
				pack.width = sw;
				pack.height = sh;
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
				pack.width = sh;
				pack.height = sw;
#endif
				PocoGrayBitmapDraw(poco, &pack, color, blend, cx, cy, 0, 0, sw, sh);
			}
		}
		x += xadvance;
		width -= xadvance;
		stringWidth -= xadvance;
	}
}

void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	Poco poco = (*self)->poco;
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		x -= sx;
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw)
		sw = tw - sx;
	if (sy < 0) {
		y -= sy;
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th)
		sh = th - sy;
	if ((sw <= 0) || (sh <= 0)) return;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(DrawTextureCommand);
		command->texture = texture;
		command->color = (*self)->pixel;
		command->blend = (*self)->blend;
		command->x = x;
		command->y = y;
		command->sx = sx;
		command->sy = sy;
		command->sw = sw;
		command->sh = sh;
	}
}

void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	Poco poco = (*self)->poco;
	PiuFlags flags = (*texture)->flags;
	PocoBitmap bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
	PocoBitmap mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
	if (mask) {
		if (bits)
			PocoBitmapDrawMasked(poco, blend, bits, x, y, sx, sy, sw, sh, mask, sx, sy);
		else
			PocoGrayBitmapDraw(poco, mask, color, blend, x, y, sx, sy, sw, sh);
	}
	else
		PocoBitmapDraw(poco, bits, x, y, sx, sy, sw, sh);
}

void PiuViewEnd(PiuView* self) 
{
	xsMachine* the = (*self)->the;
	Poco poco = (*self)->poco;
	PiuRegion* dirty = (*self)->dirty;
	PocoCoordinate *regionBegin0, *region0, *regionLimit0;
	int size, offset;

	PIUQueueCommand(EndCommand);
#ifdef mxInstrument
	modInstrumentationMax(PiuCommandListUsed, (*self)->current);
#endif
	regionBegin0 = (*dirty)->data;
	offset = 5;
	size = *regionBegin0 - 2; // last span
	region0 = regionBegin0 + offset;
	regionLimit0 = regionBegin0 + size;
	
	while (region0 < regionLimit0) {
		PocoCoordinate top = *region0++;
		PocoCoordinate segmentCount = *region0++;
		PocoCoordinate bottom = *(region0 + segmentCount);		
		while (segmentCount) {
			PocoCoordinate left = *region0++;
			segmentCount--;
			PocoCoordinate right = *region0++;
			segmentCount--;
			offset = region0 - regionBegin0;
			PiuViewUpdateStep(self, left, top, right - left, bottom - top, (region0 < regionLimit0) ? 1 : 0);
			regionBegin0 = (*dirty)->data;
			region0 = regionBegin0 + offset;
			regionLimit0 = regionBegin0 + size;
		}
	}
	
	PiuRegionEmpty(dirty);

	if (poco->flags & kPocoFlagGCDisabled) {
		xsEnableGarbageCollection(1);
		xsCollectGarbage();
		poco->flags &= ~kPocoFlagGCDisabled;
	}
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	Poco poco = (*self)->poco;
	if ((w <= 0) || (h <= 0)) return;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(FillColorCommand);
		command->color = (*self)->pixel;
		command->blend = (*self)->blend;
		command->x = x;
		command->y = y;
		command->w = w;
		command->h = h;
	}
}

void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	Poco poco = (*self)->poco;
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		if (w == sw) {
			x -= sx;
			w += sx;
		}
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw) {
		if (w == sw)
			w = tw - sx;
		sw = tw - sx;
	}
	if (sy < 0) {
		if (h == sh) {
			y -= sy;
			h += sy;
		}
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th) {
		if (h == sh)
			h = th - sy;
		sh = th - sy;
	}
	if ((w <= 0) || (h <= 0) || (sw <= 0) || (sh <= 0)) return;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(FillTextureCommand);
		command->texture = texture;
		command->color = (*self)->pixel;
		command->blend = (*self)->blend;
		command->x = x;
		command->y = y;
		command->w = w;
		command->h = h;
		command->sx = sx;
		command->sy = sy;
		command->sw = sw;
		command->sh = sh;
	}
}

void PiuViewFillTextureAux(PiuView* self, PiuTexture* texture, PocoColor color, uint8_t blend, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	Poco poco = (*self)->poco;
	PiuFlags flags = (*texture)->flags;
	PocoBitmap bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
	PocoBitmap mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
	if (mask) {
		if (bits) {
			PocoCoordinate xx, ww;
			while (h >= sh) {
				xx = x;
				ww = w;
				while (ww >= sw) {
					PocoBitmapDrawMasked(poco, blend, bits, xx, y, sx, sy, sw, sh, mask, sx, sy);
					xx += sw;
					ww -= sw;
				}
				if (ww)
					PocoBitmapDrawMasked(poco, blend, bits, xx, y, sx, sy, ww, sh, mask, sx, sy);
				y += sh;
				h -= sh;
			}
			if (h) {
				while (w >= sw) {
					PocoBitmapDrawMasked(poco, blend, bits, x, y, sx, sy, sw, h, mask, sx, sy);
					x += sw;
					w -= sw;
				}
				if (w)
					PocoBitmapDrawMasked(poco, blend, bits, x, y, sx, sy, w, h, mask, sx, sy);
			}
		}
		else {
			PocoCoordinate xx, ww;
			while (h >= sh) {
				xx = x;
				ww = w;
				while (ww >= sw) {
					PocoGrayBitmapDraw(poco, mask, color, kPocoOpaque, xx, y, sx, sy, sw, sh);
					xx += sw;
					ww -= sw;
				}
				if (ww)
					PocoGrayBitmapDraw(poco, mask, color, kPocoOpaque, xx, y, sx, sy, ww, sh);
				y += sh;
				h -= sh;
			}
			if (h) {
				while (w >= sw) {
					PocoGrayBitmapDraw(poco, mask, color, kPocoOpaque, x, y, sx, sy, sw, h);
					x += sw;
					w -= sw;
				}
				if (w)
					PocoGrayBitmapDraw(poco, mask, color, kPocoOpaque, x, y, sx, sy, w, h);
			}
		}
	}
	else {
		PocoBitmapPattern(poco, bits, x, y, w, h, sx, sy, sw, sh);
	}
}

void PiuViewGetSize(PiuView* self, PiuDimension *width, PiuDimension *height)
{
	Poco poco = (*self)->poco;
	*width = poco->width;
	*height = poco->height;
}

void PiuViewIdleCheck(PiuView* self, PiuBoolean idle)
{
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	PiuViewCombine(self, area, piuRegionUnionOp);
}

void PiuViewInvalidateRegion(PiuView* self, PiuRegion* region) 
{
	PiuViewCombineRegion(self, region, piuRegionUnionOp);
}

void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuView self = it;
	uint32_t current = sizeof(PiuViewRecord);
	uint32_t limit = self->current;
	PiuMarkHandle(the, self->dirty);
	PiuMarkHandle(the, self->swap);
	PiuMarkReference(the, self->screen);
    PiuMarkReference(the, self->pixels);
    PiuMarkReference(the, self->rectangle);
	while (current < limit) {
		uint8_t* command = ((uint8_t*)self) + current;
		switch(*((PiuCommandID*)command)) {
		case piuDrawFrameCommand:
			current += sizeof(PiuDrawFrameCommand);
			break;
		case piuDrawStringCommand:
			PiuMarkString(the, ((PiuDrawStringCommand*)command)->string);
			current += sizeof(PiuDrawStringCommand);
			break;
		case piuDrawTextureCommand:
			current += sizeof(PiuDrawTextureCommand);
			break;
		case piuFillColorCommand:
			current += sizeof(PiuFillColorCommand);
			break;
		case piuFillTextureCommand:
			current += sizeof(PiuFillTextureCommand);
			break;
		case piuPopClipCommand:
			current += sizeof(PiuPopClipCommand);
			break;
		case piuPushClipCommand:
			current += sizeof(PiuPushClipCommand);
			break;
		case piuEndCommand:
			current += sizeof(PiuEndCommand);
			break;
		}
	}
}

void PiuViewPopClip(PiuView* self)
{
	PIUQueueCommand(PopClipCommand);
}

void PiuViewPopColor(PiuView* self)
{
}

void PiuViewPopColorFilter(PiuView* self)
{
}

void PiuViewPopOrigin(PiuView* self)
{
	Poco poco = (*self)->poco;
	PocoOriginPop(poco);
}

void PiuViewPushClip(PiuView* self, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	Poco poco = (*self)->poco;
	x += poco->xOrigin;
	y += poco->yOrigin;
	{
		PIUQueueCommand(PushClipCommand);
		command->x = x;
		command->y = y;
		command->w = w;
		command->h = h;
	}
}

void PiuViewPushColor(PiuView* self, PiuColor color)
{
	(*self)->pixel = PocoMakeColor((*self)->poco, color->r, color->g, color->b);
	(*self)->blend = color->a;
}

void PiuViewPushColorFilter(PiuView* self, PiuColor color)
{
	(*self)->pixel = PocoMakeColor((*self)->poco, color->r, color->g, color->b);
	(*self)->blend = color->a;
}

void PiuViewPushOrigin(PiuView* self, PocoCoordinate x, PocoCoordinate y)
{
	Poco poco = (*self)->poco;
	PocoOriginPush(poco, x, y);
}

void PiuViewReceiver(PocoPixel *pixels, int byteLength, void *refCon)
{
	PiuView* self = refCon;
	xsMachine *the = (*self)->the;
	Poco poco = (*self)->poco;
	xsCallFunction3((*self)->_send, xsReference((*self)->screen), xsReference((*self)->pixels), xsInteger((char *)pixels - (char *)poco->pixels), xsInteger(byteLength));
}

PiuTick PiuViewTicks(PiuView* self)
{
#if defined (__ZEPHYR__) || defined (__ets__) || defined (ESP32) || defined(gecko) || defined (_RENESAS_SYNERGY_)
	return modMilliseconds();
#else
	//@@ should use modMilliseconds on all platforms
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	return (PiuTick)(((double)(tv.tv_sec) * 1000.0) + ((double)(tv.tv_usec) / 1000.0));
#endif
}

void PiuViewUpdate(PiuView* self, PiuApplication* application)
{
	PiuCoordinate* data = (*((*self)->dirty))->data;
	PiuRectangleRecord area;
	PiuRectangleSet(&area, data[1], data[2], data[3], data[4]);
	if (!PiuRectangleIsEmpty(&area)) {
		PiuViewBegin(self);
		(*(*application)->dispatch->update)(application, self, &area);
		PiuViewEnd(self);
	}
}

void PiuViewUpdateStep(PiuView* self, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, uint8_t flag) 
{
#if (defined(__GNUC__) && defined(__OPTIMIZE__)) || defined(__llvm__)
	static void *const gxDispatches[] ICACHE_XS6RO_ATTR = {
		&&PIUDrawFrameCommand,
		&&PIUDrawStringCommand,
		&&PIUDrawTextureCommand,
		&&PIUFillColorCommand,
		&&PIUFillTextureCommand,
		&&PIUPopClipCommand,
		&&PIUPushClipCommand,
		&&PIUEndCommand,
	};
	register void *const *dispatches = gxDispatches;
#endif
	xsMachine *the = (*self)->the;
	Poco poco = (*self)->poco;
	PixelsOutDispatch pixelsOutDispatch = poco->outputRefcon ? *(PixelsOutDispatch *)poco->outputRefcon : NULL;
	uint32_t current = sizeof(PiuViewRecord);
	int result;

#if kPocoFrameBuffer
	if (!(poco->flags & kPocoFlagFrameBuffer))
#endif
	{
		PocoDrawingBegin(poco, x, y, w, h);
		if ((0 == poco->w) || (0 == poco->h)) {
			PocoDrawingEnd(poco, poco->pixels, poco->pixelsLength, PiuViewReceiver, self);
			return;
		}
	}
#if kPocoFrameBuffer
	else {
		if (NULL == poco->frameBuffer) {
			PocoPixel *pixels;
			int16_t rowBytes;

			if (pixelsOutDispatch)
				(pixelsOutDispatch->doBeginFrameBuffer)(poco->outputRefcon, &pixels, &rowBytes);
			else {
				xsResult = xsCallFunction4((*self)->_begin, xsReference((*self)->screen), xsInteger(poco->x), xsInteger(poco->y), xsInteger(poco->w), xsInteger(poco->h));
				pixels = xsGetHostData(xsResult);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
				rowBytes = (int16_t)(xsToInteger(xsGet(xsResult, xsID_byteLength)) / poco->height);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
				rowBytes = (int16_t)(xsToInteger(xsGet(xsResult, xsID_byteLength)) / poco->width);
#endif
			}

			PocoDrawingBeginFrameBuffer(poco, 0, 0, poco->width, poco->height, pixels, rowBytes);
		}

		PocoClipPush(poco, x, y, w, h);
		if ((0 == poco->w) || (0 == poco->h))
			goto endStepFrameBuffer;
	}
#endif

	while (1) {
		PIUSwitch(*((PiuCommandID*)(((uint8_t*)(*self)) + current))) {
		PIUCase(DrawFrameCommand)
			PocoDrawFrame(poco, command->data, command->dataSize, command->x, command->y, command->sw, command->sh);
			PIUBreak;
		PIUCase(DrawStringCommand)
			PiuViewDrawStringAux(self, command->string, command->offset, command->length, command->font, command->color, command->blend, command->x, command->y, command->w, command->sw);
			PIUBreak;
		PIUCase(DrawTextureCommand)
			PiuViewDrawTextureAux(self, command->texture, command->color, command->blend, command->x, command->y, command->sx, command->sy, command->sw, command->sh);
			PIUBreak;
		PIUCase(FillColorCommand)
			PocoRectangleFill(poco, command->color, command->blend, command->x, command->y, command->w, command->h);
			PIUBreak;
		PIUCase(FillTextureCommand)
			PiuViewFillTextureAux(self, command->texture, command->color, command->blend, command->x, command->y, command->w, command->h, command->sx, command->sy, command->sw, command->sh);
			PIUBreak;
		PIUCase(PopClipCommand)
#if defined(__GNUC__)
			#pragma unused (command)
#endif
			PocoClipPop(poco);
			PIUBreak;
		PIUCase(PushClipCommand)
			PocoClipPush(poco, command->x, command->y, command->w, command->h);
			if ((0 == poco->w) || (0 == poco->h)) {
				uint8_t depth = 1;
				do {
					switch (*((PiuCommandID*)(((uint8_t*)(*self)) + current))) {
						case piuDrawFrameCommand:		current += sizeof(PiuDrawFrameCommand); break;
						case piuDrawStringCommand:		current += sizeof(PiuDrawStringCommand); break;
						case piuDrawTextureCommand:		current += sizeof(PiuDrawTextureCommand); break;
						case piuFillColorCommand:		current += sizeof(PiuFillColorCommand); break;
						case piuFillTextureCommand:		current += sizeof(PiuFillTextureCommand); break;
						case piuPopClipCommand:			current += sizeof(PiuPopClipCommand); depth -= 1;	break;
						case piuPushClipCommand:		current += sizeof(PiuPushClipCommand);  depth += 1;	break;
					}
				} while (depth);
				PocoClipPop(poco);
			}
			PIUBreak;
		PIUCase(EndCommand)
#if defined(__GNUC__)
			#pragma unused (command)
#endif
			goto done;
			PIUBreak;
		}
	}
done:

#if kPocoFrameBuffer
	if (!(poco->flags & kPocoFlagFrameBuffer))
#endif
	{
		if (pixelsOutDispatch) {
			(pixelsOutDispatch->doBegin)(poco->outputRefcon, poco->x, poco->y, poco->w, poco->h);
			result = PocoDrawingEnd(poco, poco->pixels, poco->pixelsLength, pixelsOutDispatch->doSend, poco->outputRefcon);
		}
		else {
			xsCallFunction4((*self)->_begin, xsReference((*self)->screen), xsInteger(poco->x), xsInteger(poco->y), xsInteger(poco->w), xsInteger(poco->h));
			result = PocoDrawingEnd(poco, poco->pixels, poco->pixelsLength, PiuViewReceiver, self);
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
		if (flag) {
			if (pixelsOutDispatch)
				(pixelsOutDispatch->doContinue)(poco->outputRefcon);
			else
				xsCallFunction0((*self)->_continue, xsReference((*self)->screen));
		}
		else {
			if (pixelsOutDispatch)
				(pixelsOutDispatch->doEnd)(poco->outputRefcon);
			else
				xsCallFunction0((*self)->_end, xsReference((*self)->screen));

			pocoInstrumentationAdjust(FramesDrawn, +1);
		}
	}
#if kPocoFrameBuffer
	else {
endStepFrameBuffer:
		PocoClipPop(poco);
		if (!flag) {
			PocoDrawingEndFrameBuffer(poco);

			if (pixelsOutDispatch)
				(pixelsOutDispatch->doEnd)(poco->outputRefcon);
			else
				xsCallFunction0((*self)->_end, xsReference((*self)->screen));

			pocoInstrumentationAdjust(FramesDrawn, +1);
		}
	}
#endif
}

void PiuViewValidate(PiuView* self, PiuRectangle area) 
{
	PiuViewCombine(self, area, piuRegionDifferenceOp);
}

void PiuViewValidateRegion(PiuView* self, PiuRegion* region) 
{
	PiuViewCombineRegion(self, region, piuRegionDifferenceOp);
}

void PiuApplication_animateColors(xsMachine* the) 
{
#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	PiuColorRecord color;
	PocoColor colors[16];
	xsIntegerValue i = 0;
	xsVars(2);
	while (i < 16) {
		xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
		PiuColorDictionary(the, &xsVar(0), &color);
		colors[i] = ((color.r >> 3) << 11) | ((color.g >> 2) << 5) | (color.b >> 3);
		i++;
	}
	xsVar(0) = xsReference((*view)->screen);
	xsVar(1) = xsNewHostObject(NULL);
	xsSetHostData(xsVar(1), colors);
	xsCall1(xsVar(0), xsID_animateColors, xsVar(1));
#endif
}

void PiuApplication_get_clut(xsMachine* the) 
{
#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	xsVars(1);
	xsVar(0) = xsReference((*view)->screen);
	xsResult = xsGet(xsVar(0), xsID_clut);
#endif
}

void PiuApplication_set_clut(xsMachine* the) 
{
#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	Poco poco = (*view)->poco;
	xsVars(1);
	xsVar(0) = xsReference((*view)->screen);
	xsSet(xsVar(0), xsID_clut, xsArg(0));
	PiuViewInvalidate(view, NULL);
	poco->clut = xsGetHostData(xsArg(0));
#endif
}

void PiuApplication_keyDown(xsMachine* the) 
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuContent* content = (*self)->focus;
	xsVars(3);
	while (content) {
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), xsID_onKeyDown)) {
				xsVar(1) = xsReference((*content)->reference);
				xsVar(2) = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsArg(0));
				if (xsTest(xsVar(2)))
					break;
			}
		}
		content = (PiuContent*)(*content)->container;
	}
}

void PiuApplication_keyUp(xsMachine* the) 
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuContent* content = (*self)->focus;
	xsVars(3);
	while (content) {
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), xsID_onKeyUp)) {
				xsVar(1) = xsReference((*content)->reference);
				xsVar(2) = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsArg(0));
				if (xsTest(xsVar(2)))
					break;
			}
		}
		content = (PiuContent*)(*content)->container;
	}
}

void PiuApplication_postMessage(xsMachine* the) 
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	xsVars(1);
	xsVar(0) = xsReference((*view)->screen);
	if (xsFindResult(xsVar(0), xsID_postMessage)) {
		(void)xsCallFunction1(xsResult, xsVar(0), xsArg(0));
	}
}

void PiuApplication_purge(xsMachine* the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuStyleLinkNew(the);
	(*self)->styleList = PIU(StyleLink, xsResult);
	xsSet(xsGlobal, xsID_assetMap, xsNull);
	xsCollectGarbage();
}

void PiuCLUT_get_colors(xsMachine* the) 
{
	uint8_t* data = (uint8_t*)xsGetHostData(xsThis);
	uint16_t* clut = (uint16_t*)(32 + 4096 + data);
	xsIntegerValue i = 0;
	xsVars(1);
	xsResult = xsNewArray(0);
	xsSet(xsResult, xsID_length, xsInteger(16));
	xsCall0(xsResult, xsID_fill);
	while (i < 16) {
		uint16_t src = *clut;
		uint8_t r = src >> 11;
		uint8_t g = (src >> 5) & 0x3F;
		uint8_t b = src & 0x1F;
		PiuColorRecord c0;
		c0.r = (r << 3) | (r >> 2);
		c0.g = (g << 2) | (g >> 4);
		c0.b = (b << 3) | (b >> 2);
		c0.a = 0xFF;
		fxUnsigned(the, &xsVar(0), ((uint32_t)c0.r << 24) | ((uint32_t)c0.g << 16) | ((uint32_t)c0.b << 8) | ((uint32_t)c0.a));
		xsSetAt(xsResult, xsInteger(i), xsVar(0));
		clut++;
		i++;
	}
}

void PiuView_create(xsMachine* the) 
{
	PiuView* self;
	PiuApplication* application;
	xsIntegerValue commandListLength, regionLength, size;
// 	xsLog("view free %d\n", system_get_free_heap_size());
	if (!xsFindInteger(xsArg(1), xsID_commandListLength, &commandListLength))
		commandListLength = 1024;
	if (!xsFindInteger(xsArg(1), xsID_regionLength, &regionLength))
		regionLength = 512;
	size = sizeof(PiuViewRecord) + commandListLength;
	xsSetHostChunk(xsThis, NULL, size);
	self = PIU(View, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, &PiuViewHooks);
	application = (*self)->application = PIU(Application, xsArg(0));
	(*application)->view = self;
	
	(*self)->poco = xsGetHostDataPoco(xsArg(3));
	PiuRegionNew(the, (PiuCoordinate)regionLength);
	(*self)->dirty = PIU(Region, xsResult);
	PiuRegionNew(the, (PiuCoordinate)regionLength);
	(*self)->swap = PIU(Region, xsResult);
	(*self)->current = sizeof(PiuViewRecord);
	(*self)->limit = size;
	(*self)->screen = xsToReference(xsArg(2));
	(*self)->pixels = xsToReference(xsArg(3));
	(*self)->rectangle = xsToReference(xsArg(4));
	(*self)->_adaptInvalid = xsGet(xsArg(2), xsID_adaptInvalid);
	(*self)->_begin = xsGet(xsArg(2), xsID_begin);
	(*self)->_continue = xsGet(xsArg(2), xsID_continue);
	(*self)->_end = xsGet(xsArg(2), xsID_end);
	(*self)->_send = xsGet(xsArg(2), xsID_send);
}

void PiuView_get_rotation(xsMachine* the) 
{
#if 0 == kPocoRotation
	xsResult = xsInteger(0);
#elif 90 == kPocoRotation
	xsResult = xsInteger(90);
#elif 180 == kPocoRotation
	xsResult = xsInteger(180);
#elif 270 == kPocoRotation
	xsResult = xsInteger(270);
#endif
}

void PiuView_onIdle(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	xsVars(2);
	PiuApplicationDeferContents(the, application);
	PiuApplicationIdleContents(application);
	PiuApplicationTouchIdle(application);
	PiuApplicationAdjust(application);
	PiuViewUpdate(self, application);
}

void PiuView_onMessage(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	if ((*application)->behavior) {
		xsVars(2);
		xsVar(0) = xsReference((*application)->behavior);
		if (xsFindResult(xsVar(0), xsID_onMessage)) {
			xsVar(1) = xsReference((*application)->reference);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsArg(0));
		}
	}
	PiuApplicationAdjust(application);
	PiuViewUpdate(self, application);
}

void PiuView_onTouchBegan(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	xsIntegerValue index = xsToInteger(xsArg(0));
	PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
	xsNumberValue ticks = xsToNumber(xsArg(3));
#if (90 == kPocoRotation)
	PiuCoordinate c = x;
	x = y;
	y = (*application)->coordinates.height - c; 
#elif (180 == kPocoRotation)
	x = (*application)->coordinates.width - x;
	y = (*application)->coordinates.height - y; 
#elif (270 == kPocoRotation)
	PiuCoordinate c = x;
	x = (*application)->coordinates.width - y;
	y = c; 
#endif
	PiuApplicationTouchBegan(application, index, x, y, ticks);
	PiuApplicationAdjust(application);
	PiuViewUpdate(self, application);
}

void PiuView_onTouchEnded(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	xsIntegerValue index = xsToInteger(xsArg(0));
	PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
	xsNumberValue ticks = xsToNumber(xsArg(3));
#if (90 == kPocoRotation)
	PiuCoordinate c = x;
	x = y;
	y = (*application)->coordinates.height - c; 
#elif (180 == kPocoRotation)
	x = (*application)->coordinates.width - x;
	y = (*application)->coordinates.height - y; 
#elif (270 == kPocoRotation)
	PiuCoordinate c = x;
	x = (*application)->coordinates.width - y;
	y = c; 
#endif
	PiuApplicationTouchEnded(application, index, x, y, ticks);
	PiuApplicationAdjust(application);
	PiuViewUpdate(self, application);
}

void PiuView_onTouchMoved(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	xsIntegerValue index = xsToInteger(xsArg(0));
	PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
	xsNumberValue ticks = xsToNumber(xsArg(3));
#if (90 == kPocoRotation)
	PiuCoordinate c = x;
	x = y;
	y = (*application)->coordinates.height - c; 
#elif (180 == kPocoRotation)
	x = (*application)->coordinates.width - x;
	y = (*application)->coordinates.height - y; 
#elif (270 == kPocoRotation)
	PiuCoordinate c = x;
	x = (*application)->coordinates.width - y;
	y = c; 
#endif
	PiuApplicationTouchMoved(application, index, x, y, ticks);
}

