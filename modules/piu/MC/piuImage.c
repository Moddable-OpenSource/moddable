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

extern const void *mcGetResource(const char* path, size_t* size);

static void PiuImageDictionary(xsMachine* the, void* it);
static void PiuImageDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuImageMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuImageMeasureHorizontally(void* it);
static void PiuImageMeasureVertically(void* it);
static void PiuImageSync(void* it);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuImageDispatchRecord = {
	"Image",
	PiuContentBind,
	PiuContentCascade,
	PiuImageDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuImageMeasureHorizontally,
	PiuImageMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuImageSync,
	PiuContentUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuImageHooks = {
	PiuContentDelete,
	PiuImageMark,
	NULL
};

void PiuImageDictionary(xsMachine* the, void* it) 
{
	PiuImage* self = it;
	if (xsFindResult(xsArg(1), xsID_path)) {
		xsSlot* path = PiuString(xsResult);
		(*self)->path = path;
	}
}

void PiuImageDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuImage* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->data) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawFrame(view, (*self)->data + sizeof(uint16_t) + (*self)->frameOffset, (*self)->frameSize, 0, 0, (*self)->dataWidth, (*self)->dataHeight);
		PiuViewPopClip(view);
	}
}

void PiuImageMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuImage self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkString(the, self->path);
}

void PiuImageMeasureHorizontally(void* it) 
{
	PiuImage* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth))
		(*self)->coordinates.width = (*self)->dataWidth;
}

void PiuImageMeasureVertically(void* it) 
{
	PiuImage* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight))
		(*self)->coordinates.height = (*self)->dataHeight;
}

void PiuImageSync(void* it)
{
	PiuImage* self = it;
	xsIntegerValue frameCount = (*self)->frameCount;
	if (frameCount > 1) {
		xsIntegerValue fromIndex = (*self)->frameIndex;
		xsIntegerValue toIndex = (xsIntegerValue)c_round((frameCount * (*self)->time) / (*self)->duration);
		if (toIndex >= frameCount)
			toIndex = frameCount - 1;
		if (fromIndex != toIndex) {
			uint8_t* data = (*self)->data;
			uint32_t frameOffset, frameSize;
			if (fromIndex > toIndex) {
				fromIndex = 0;
				frameOffset = sizeof(ColorCellHeaderRecord);
				frameSize = c_read16(data + frameOffset);
			}
			else {
				frameOffset = (*self)->frameOffset;
				frameSize = (*self)->frameSize;
			}
			while (fromIndex < toIndex) {
				fromIndex++;
				frameOffset += sizeof(uint16_t) + frameSize;
				frameSize = c_read16(data + frameOffset);
		
			}
			(*self)->frameIndex = toIndex;
			(*self)->frameOffset = frameOffset;
			(*self)->frameSize = frameSize;
			PiuContentInvalidate(self, NULL);
		}
	}
}

void PiuImage_create(xsMachine* the)
{
	PiuImage* self;
	xsStringValue path;
	uint8_t* data;
	size_t dataSize;
	xsIntegerValue frameCount;
	ColorCellHeader cch;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuImageRecord));
	self = PIU(Image, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuImageHooks);
	(*self)->dispatch = (PiuDispatch)&PiuImageDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuImageDictionary(the, self);
	path = PiuToString((*self)->path);
	data = (uint8_t *)mcGetResource(path, &dataSize);
	if (!data)
		xsURIError("image not found: %s", path);	
	cch = (ColorCellHeader)data;
	if (('c' != c_read8(&cch->id_c)) || ('s' != c_read8(&cch->id_s)))
		xsUnknownError("invalid image data");
	if (kCommodettoBitmapRGB565LE != c_read8(&cch->bitmapFormat))
		xsUnknownError("invalid image pixel format");
	if (0 != c_read8(&cch->reserved))
		xsUnknownError("invalid image reserved");
	(*self)->data = data;
	(*self)->dataSize = (uint32_t)dataSize;
#if (90 == kPocoRotation) || (270 == kPocoRotation)
	(*self)->dataWidth = c_read16(&cch->height);
	(*self)->dataHeight = c_read16(&cch->width);
#else
	(*self)->dataWidth = c_read16(&cch->width);
	(*self)->dataHeight = c_read16(&cch->height);
#endif
	(*self)->frameIndex = 0;
	(*self)->frameOffset = sizeof(ColorCellHeaderRecord);
	(*self)->frameSize = c_read16(data + sizeof(ColorCellHeaderRecord));
	frameCount = (*self)->frameCount = c_read16(&cch->frameCount);
	if (frameCount > 1) {
		uint16_t fps_numerator = c_read16(&cch->fps_numerator);
		uint16_t fps_denominator = c_read16(&cch->fps_denominator);
		(*self)->duration = (1000 * (double)frameCount * (double)fps_numerator ) / (double)fps_denominator;
		(*self)->interval = (1000 * (PiuInterval)fps_numerator ) / (PiuInterval)fps_denominator;
		(*self)->time = 0;
	}
	PiuBehaviorOnCreate(self);
}

void PiuImage_get_frameCount(xsMachine *the)
{
	PiuImage* self = PIU(Image, xsThis);
	xsResult = xsInteger((*self)->frameCount);
}

void PiuImage_get_frameIndex(xsMachine *the)
{
	PiuImage* self = PIU(Image, xsThis);
	xsResult = xsInteger((*self)->frameIndex);
}

void PiuImage_set_frameIndex(xsMachine *the)
{
	PiuImage* self = PIU(Image, xsThis);
	xsIntegerValue frameCount = (*self)->frameCount;
	if (frameCount > 1) {
		xsIntegerValue frameIndex = xsToInteger(xsArg(0));
		if (frameIndex < 0)
			frameIndex = 0;
		else if (frameIndex >= frameCount)
			frameIndex = frameCount - 1;
		PiuContentSetTime((PiuContent*)self, ((double)frameIndex * (*self)->duration) / (double)frameCount);
	}
}


