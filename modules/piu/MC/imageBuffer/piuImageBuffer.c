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

#include "piuMC.h"

typedef struct PiuImageBufferStruct PiuImageBufferRecord, *PiuImageBuffer;

struct PiuImageBufferStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* buffer;
	PocoBitmapRecord bits;
	xsIntegerValue imageWidth;
	xsIntegerValue imageHeight;
};

static void PiuImageBufferDictionary(xsMachine* the, void* it);
static void PiuImageBufferDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuImageBufferDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuImageBufferMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuImageBufferMeasureHorizontally(void* it);
static void PiuImageBufferMeasureVertically(void* it);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuImageBufferDispatchRecord = {
	"ImageBuffer",
	PiuContentBind,
	PiuContentCascade,
	PiuImageBufferDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuImageBufferMeasureHorizontally,
	PiuImageBufferMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuContentUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuImageBufferHooks = {
	PiuContentDelete,
	PiuImageBufferMark,
	NULL
};

void PiuImageBufferDictionary(xsMachine* the, void* it) 
{
	PiuImageBuffer* self = it;
	xsIntegerValue integer;
	if (xsFindInteger(xsArg(1), xsID_imageType, &integer)) {
		if ((integer != kCommodettoBitmapFormat) && (integer != kCommodettoBitmapGray16))
			xsUnknownError("invalid imageType");
		(*self)->bits.format = integer;
	}
	else
		(*self)->bits.format = kCommodettoBitmapFormat;
	if (xsFindInteger(xsArg(1), xsID_imageWidth, &integer))
		(*self)->imageWidth = integer;
	else
		xsUnknownError("no imageWidth");
	if (xsFindInteger(xsArg(1), xsID_imageHeight, &integer))
		(*self)->imageHeight = integer;
	else
		xsUnknownError("no imageHeight");
	#if (90 == kPocoRotation) || (270 == kPocoRotation)
		(*self)->bits.width = (*self)->imageHeight;
		(*self)->bits.height = (*self)->imageWidth;
	#else
		(*self)->bits.width = (*self)->imageWidth;
		(*self)->bits.height = (*self)->imageHeight;
	#endif
}

void PiuImageBufferDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuImageBuffer* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->buffer) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawContent(view, PiuImageBufferDrawAux, it, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewPopClip(view);
	}
}

mxImport xsIntegerValue _xsmcGetBuffer(xsMachine *the, xsSlot *slot, void **data, xsUnsignedValue *count, xsBooleanValue writable);

void PiuImageBufferDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuImageBuffer* self = it;
	void* pixels;
	xsUnsignedValue count;
	xsBeginHost((*self)->the);
	xsVars(1);
	xsVar(0) = xsReference((*self)->buffer);
	_xsmcGetBuffer(the, &(xsVar(0)), &pixels, &count, 0);
	(*self)->bits.pixels = pixels;
	x += (*view)->poco->xOrigin;
	y += (*view)->poco->yOrigin;
	if (kCommodettoBitmapGray16 == (*self)->bits.format)
		PocoGrayBitmapDraw((*view)->poco, &((*self)->bits), PocoMakeColor((*view)->poco, 0, 0, 0), kPocoOpaque, x, y, 0, 0, sw, sh);
	else
		PocoBitmapDraw((*view)->poco, &((*self)->bits), x, y, 0, 0, sw, sh);
	xsEndHost((*self)->the);
}

void PiuImageBufferMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuImageBuffer self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkReference(the, self->buffer);
}

void PiuImageBufferMeasureHorizontally(void* it) 
{
	PiuImageBuffer* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth))
		(*self)->coordinates.width = (*self)->imageWidth;
}

void PiuImageBufferMeasureVertically(void* it) 
{
	PiuImageBuffer* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight))
		(*self)->coordinates.height = (*self)->imageHeight;
}

void PiuImageBuffer_create(xsMachine* the)
{
	PiuImageBuffer* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuImageBufferRecord));
	self = PIU(ImageBuffer, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuImageBufferHooks);
	(*self)->dispatch = (PiuDispatch)&PiuImageBufferDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuImageBufferDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuImageBuffer_get_buffer(xsMachine *the)
{
	PiuImageBuffer* self = PIU(ImageBuffer, xsThis);
	xsResult = xsReference((*self)->buffer);
}

void PiuImageBuffer_set_buffer(xsMachine *the)
{
	PiuImageBuffer* self = PIU(ImageBuffer, xsThis);
	(*self)->buffer = xsToReference(xsArg(0));
	PiuContentInvalidate(self, NULL);
}


