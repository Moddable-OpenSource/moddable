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

#include "commodettoReadGIF.h"
#include "piuMC.h"

typedef struct PiuGIFImageStruct PiuGIFImageRecord, *PiuGIFImage;
struct PiuGIFImageStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* path;
	xsSlot* buffer;
	xsSlot* flip;
	PiuColorRecord color;
	xsSlot* gif;
	PiuDimension gifWidth;
	PiuDimension gifHeight;
	int frameCount;
	double frameTime;
};

enum {
	piuColorized = 1 << 24,
};

static void PiuGIFImageBind(void* it, PiuApplication* application, PiuView* view);
static void PiuGIFImageDictionary(xsMachine* the, void* it);
static void PiuGIFImageDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuGIFImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuGIFImageMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuGIFImageMeasureHorizontally(void* it);
static void PiuGIFImageMeasureVertically(void* it);
static void PiuGIFImageSync(void* it);
static void PiuGIFImageUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuGIFImageDispatchRecord = {
	"GIFImage",
	PiuGIFImageBind,
	PiuContentCascade,
	PiuGIFImageDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuGIFImageMeasureHorizontally,
	PiuGIFImageMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuGIFImageSync,
	PiuGIFImageUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuGIFImageHooks = {
	PiuContentDelete,
	PiuGIFImageMark,
	NULL
};

void PiuGIFImageBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuGIFImage* self = it;
	if ((*self)->path || (*self)->buffer) {
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->reference);
		if ((*self)->path)
			xsVar(1) = *((*self)->path);
		else
			xsVar(1) = xsReference((*self)->buffer);
		xsResult = xsCall2(xsVar(0), xsID_load, xsVar(1), ((*self)->flags & piuColorized) ? xsTrue : xsFalse);
		(*self)->gif = xsToReference(xsResult);
		(*self)->gifWidth = xsToInteger(xsGet(xsResult, xsID_width));
		(*self)->gifHeight = xsToInteger(xsGet(xsResult, xsID_height));
		(*self)->frameCount = xsToInteger(xsGet(xsResult, xsID_frameCount));
		xsCall0(xsResult, xsID_first);
		xsEndHost((*self)->the);
	}
	(*self)->frameTime = 0;
	PiuContentBind(it, application, view);
}

void PiuGIFImageDictionary(xsMachine* the, void* it) 
{
	PiuGIFImage* self = it;
	if (xsFindResult(xsArg(1), xsID_path)) {
		xsSlot* path = PiuString(xsResult);
		(*self)->path = path;
	}
	else if (xsFindResult(xsArg(1), xsID_buffer)) {
		xsSlot* buffer = xsToReference(xsResult);
		(*self)->buffer = buffer;
	}
	if (xsFindResult(xsArg(1), xsID_flip)) {
		xsSlot* flip = PiuString(xsResult);
		(*self)->flip = flip;
	}
	if (xsFindResult(xsArg(1), xsID_color)) {
		PiuColorRecord color;
		PiuColorDictionary(the, &xsResult, &color);
		(*self)->color = color;
		(*self)->flags |= piuColorized;
	}
}

void PiuGIFImageDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuGIFImage* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->gif) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawContent(view, PiuGIFImageDrawAux, it, 0, 0, bounds.width, bounds.height);
		PiuViewPopClip(view);
	}
}

void PiuGIFImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuGIFImage* self = it;
	xsBeginHost((*self)->the);
	xsVars(4);
	xsVar(0) = xsReference((*view)->reference);
	xsVar(0) = xsGet(xsVar(0), xsID_poco);
	xsVar(1) = xsReference((*self)->gif);
	if ((*self)->flags & piuColorized) {
		PiuColor color = &((*self)->color);
		xsVar(2) = xsInteger(PocoMakeColor((*view)->poco, color->r, color->g, color->b));
		xsCall4(xsVar(0), xsID_drawGray, xsVar(1), xsVar(2), xsInteger(x), xsInteger(y));
	}
	else {
		xsVar(2) = xsGet(xsVar(1), xsID_transparentColor);
		if ((*self)->flip)
			xsVar(3) = *((*self)->flip);
		xsCall5(xsVar(0), xsID_drawBitmapWithKeyColor, xsVar(1), xsInteger(x), xsInteger(y), xsVar(2), xsVar(3));
	}
	xsEndHost((*self)->the);
}

void PiuGIFImageMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuGIFImage self = it;
	PiuContentMark(the, it, markRoot);
	if (self->path)
		PiuMarkString(the, self->path);
	if (self->buffer)
		PiuMarkReference(the, self->buffer);
	if (self->flip)
		PiuMarkString(the, self->flip);
	PiuMarkReference(the, self->gif);
}

void PiuGIFImageMeasureHorizontally(void* it) 
{
	PiuGIFImage* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth))
		(*self)->coordinates.width = (*self)->gifWidth;
}

void PiuGIFImageMeasureVertically(void* it) 
{
	PiuGIFImage* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight))
		(*self)->coordinates.height = (*self)->gifHeight;
}

void PiuGIFImageSync(void* it)
{
	PiuGIFImage* self = it;
	if ((*self)->gif && ((*self)->frameCount > 1)) {
		double time, frameTime;
		PiuRectangleRecord r, u;
		xsBeginHost((*self)->the);
		xsVars(1);
		xsVar(0) = xsReference((*self)->gif);
		time = (*self)->time;
		frameTime = (*self)->frameTime;
		if (frameTime > time) {
			(*self)->frameTime = frameTime = 0;
			xsCall0(xsVar(0), xsID_first);
			u.x = xsToPiuCoordinate(xsGet(xsVar(0), xsID_frameX));
			u.y = xsToPiuCoordinate(xsGet(xsVar(0), xsID_frameY));
			u.width = xsToPiuDimension(xsGet(xsVar(0), xsID_frameWidth));
			u.height = xsToPiuDimension(xsGet(xsVar(0), xsID_frameHeight));
		}
		else
			PiuRectangleEmpty(&u);
		frameTime += xsToInteger(xsGet(xsVar(0), xsID_frameDuration));
		while (frameTime < time) {
			xsCall0(xsVar(0), xsID_next);
			r.x = xsToPiuCoordinate(xsGet(xsVar(0), xsID_frameX));
			r.y = xsToPiuCoordinate(xsGet(xsVar(0), xsID_frameY));
			r.width = xsToPiuDimension(xsGet(xsVar(0), xsID_frameWidth));
			r.height = xsToPiuDimension(xsGet(xsVar(0), xsID_frameHeight));
			PiuRectangleUnion(&u, &u, &r);
			(*self)->frameTime = frameTime;
			frameTime += xsToInteger(xsGet(xsVar(0), xsID_frameDuration));
		}
		xsEndHost((*self)->the);
		if (!PiuRectangleIsEmpty(&u))
			PiuContentInvalidate(self, &u);
	}	
}

void PiuGIFImageUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuGIFImage* self = it;
	PiuContentUnbind(it, application, view);
	if ((*self)->gif) {
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->reference);
		xsVar(1) = xsReference((*self)->gif);
		xsCall1(xsVar(0), xsID_unload, xsVar(1));
		(*self)->frameCount = 0;
		(*self)->gifHeight= 0;
		(*self)->gifWidth = 0;
		(*self)->gif = NULL;
		xsEndHost((*self)->the);
	}
}

void PiuGIFImage_create(xsMachine* the)
{
	PiuGIFImage* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuGIFImageRecord));
	self = PIU(GIFImage, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuGIFImageHooks);
	(*self)->dispatch = (PiuDispatch)&PiuGIFImageDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuGIFImageDictionary(the, self);
	PiuBehaviorOnCreate(self);
}


