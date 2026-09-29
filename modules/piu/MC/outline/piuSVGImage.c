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
#include "commodettoPocoOutline.h"

typedef struct PiuSVGImageStruct PiuSVGImageRecord, *PiuSVGImage;
struct PiuSVGImageStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* path;
	xsSlot* resource;
	xsSlot* data;
	PiuDimension dataWidth;
	PiuDimension dataHeight;
	int frameCount;
	int frameIndex;
	xsNumberValue o;
	xsNumberValue cx;
	xsNumberValue cy;
	xsNumberValue r;
	xsNumberValue sx;
	xsNumberValue sy;
	xsNumberValue tx;
	xsNumberValue ty;
	xsBooleanValue transforming;
};

static void PiuSVGImageBind(void* it, PiuApplication* application, PiuView* view);
static void PiuSVGImageDictionary(xsMachine* the, void* it);
static void PiuSVGImageDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuSVGImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuSVGImageInvalidate(void* it, PiuRectangle area);
static void PiuSVGImageMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuSVGImageMeasureHorizontally(void* it);
static void PiuSVGImageMeasureVertically(void* it);
static void PiuSVGImageSync(void* it);
static void PiuSVGImageUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuSVGImageDispatchRecord = {
	"SVGImage",
	PiuSVGImageBind,
	PiuContentCascade,
	PiuSVGImageDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuSVGImageInvalidate,
	PiuSVGImageMeasureHorizontally,
	PiuSVGImageMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuSVGImageSync,
	PiuSVGImageUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuSVGImageHooks = {
	PiuContentDelete,
	PiuSVGImageMark,
	NULL
};

void PiuSVGImageBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuSVGImage* self = it;
	if ((*self)->path || (*self)->resource) {
		xsBeginHost((*self)->the);
		xsVars(3);
		xsVar(0) = xsReference((*self)->reference);
		if ((*self)->path) {
			char path[C_PATH_MAX], *dot;
			xsVar(1) = *((*self)->path);
			xsToStringBuffer(xsVar(1), path, sizeof(path));
			dot = c_strrchr(path, '.');
			if (dot && (!c_strcmp(dot, ".svg") || !c_strcmp(dot, ".pdc")))
				c_strcpy(dot, ".js");
			xsVar(1) = xsAwaitImport(path, XS_IMPORT_DEFAULT);
		}
		else
			xsVar(1) = xsReference((*self)->resource);
		xsResult = xsCall1(xsVar(0), xsID_load, xsVar(1));
		(*self)->data = xsToReference(xsResult);
		(*self)->dataWidth = xsToInteger(xsGet(xsResult, xsID_width));
		(*self)->dataHeight = xsToInteger(xsGet(xsResult, xsID_height));
		xsVar(1) = xsGet(xsResult, xsID_frames);
		(*self)->frameCount = xsToInteger(xsGet(xsVar(1), xsID_length));
		(*self)->cx = (xsNumberValue)(*self)->dataWidth / 2.0;
		(*self)->cy = (xsNumberValue)(*self)->dataHeight / 2.0;
		(*self)->sx = (*self)->sy = 1;
		(*self)->o = 1;
		(*self)->transforming = 1;
		xsEndHost((*self)->the);
	}
	PiuContentBind(it, application, view);
}

void PiuSVGImageDictionary(xsMachine* the, void* it) 
{
	PiuSVGImage* self = it;
	xsBooleanValue boolean;
	if (xsFindBoolean(xsArg(1), xsID_clip, &boolean)) {
		if (boolean)
			(*self)->flags |= piuClip;
		else
			(*self)->flags &= ~piuClip;
	}
	if (xsFindResult(xsArg(1), xsID_path)) {
		xsSlot* path = PiuString(xsResult);
		(*self)->path = path;
	}
	else if (xsFindResult(xsArg(1), xsID_resource)) {
		xsSlot* resource = xsToReference(xsResult);
		(*self)->resource = resource;
	}
}

void PiuSVGImageDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuSVGImage* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->data) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		if ((*self)->flags & piuClip)
			PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawContent(view, PiuSVGImageDrawAux, it, 0, 0, bounds.width, bounds.height);
		if ((*self)->flags & piuClip)
			PiuViewPopClip(view);
	}
}

void PiuSVGImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuSVGImage* self = it;
	xsBeginHost((*self)->the);
	xsVars(3);
	xsVar(0) = xsReference((*self)->data);
	xsVar(0) = xsGet(xsVar(0), xsID_frames);
	xsVar(0) = xsGetIndex(xsVar(0), (*self)->frameIndex);
	xsVar(0) = xsGet(xsVar(0), xsID_commands);
	xsIntegerValue index = 0;
	xsIntegerValue length = xsToInteger(xsGet(xsVar(0), xsID_length));
	while (index < length) {
		xsVar(1) = xsGetIndex(xsVar(0), index);
		if ((*self)->transforming) {
			xsVar(2) = xsGet(xsVar(1), xsID_outline);
			xsVar(2) = xsCall0(xsVar(2), xsID_clone);
			xsVar(2) = xsCall7(xsVar(2), xsID_transform,
				xsNumber((*self)->cx),
				xsNumber((*self)->cy),
				xsNumber((*self)->r),
				xsNumber((*self)->sx),
				xsNumber((*self)->sy),
				xsNumber((*self)->tx),
				xsNumber((*self)->ty)
			);
			xsSet(xsVar(1), xsID_clone, xsVar(2));
		}
		else
			xsVar(2) = xsGet(xsVar(1), xsID_clone);
		PocoOutline outline = xsGetHostData(xsVar(2));
		xsResult = 	xsGet(xsVar(1), xsID_color);
		PiuColorRecord color;
		PiuColorDictionary(the, &xsResult, &color);
		PocoOutlineFill((*view)->poco,
			PocoMakeColor((*view)->poco, color.r, color.g, color.b),
			(uint8_t)c_round(color.a * (*self)->o),
			outline,
			(PiuCoordinate)c_round(x + (*self)->cx),
			(PiuCoordinate)c_round(y + (*self)->cy));
		index++;
	}
	(*self)->transforming = 0;
	xsEndHost((*self)->the);
}

void PiuSVGImageInvalidate(void* it, PiuRectangle area) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuVisible) {
		PiuContainer* container = (*self)->container;
		if (container) {
			PiuRectangleRecord bounds;
			if ((*self)->flags & piuClip)
				PiuRectangleSet(&bounds, (*self)->bounds.x, (*self)->bounds.y, (*self)->bounds.width, (*self)->bounds.height);
			else {
				PiuApplication* application = ((*self)->application);
				PiuRectangleSet(&bounds, (*application)->bounds.x, (*application)->bounds.y, (*application)->bounds.width, (*application)->bounds.height);
			}
			(*(*container)->dispatch->invalidate)((*self)->container, &bounds);
		}
	}
}

void PiuSVGImageMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuSVGImage self = it;
	PiuContentMark(the, it, markRoot);
	if (self->path)
		PiuMarkString(the, self->path);
	if (self->resource)
		PiuMarkReference(the, self->resource);
	if (self->data)
		PiuMarkReference(the, self->data);
}

void PiuSVGImageMeasureHorizontally(void* it) 
{
	PiuSVGImage* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth))
		(*self)->coordinates.width = (*self)->dataWidth;
}

void PiuSVGImageMeasureVertically(void* it) 
{
	PiuSVGImage* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight))
		(*self)->coordinates.height = (*self)->dataHeight;
}

void PiuSVGImageSync(void* it)
{
	PiuSVGImage* self = it;
	if ((*self)->data && ((*self)->frameCount > 1)) {
		double time, frameTime;
		int frameIndex;
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->data);
		xsVar(0) = xsGet(xsVar(0), xsID_frames);
		time = PiuContentUseIdle(it)->time;
		frameIndex = (*self)->frameIndex;
		xsVar(1) = xsGetIndex(xsVar(0), frameIndex);
		frameTime = xsToNumber(xsGet(xsVar(1), xsID_time));
		if (frameTime > time)
			frameIndex = 1;
		while (frameIndex < (*self)->frameCount) {
			xsVar(1) = xsGetIndex(xsVar(0), frameIndex);
			frameTime = xsToNumber(xsGet(xsVar(1), xsID_time));
			if (time < frameTime)
				break;
			frameIndex++;
		}
		frameIndex--;
		if ((*self)->frameIndex != frameIndex) {
			(*self)->frameIndex = frameIndex;
			(*self)->transforming = 1;
			PiuSVGImageInvalidate(self, NULL);
		}
		xsEndHost((*self)->the);
	}	
}

void PiuSVGImageUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuSVGImage* self = it;
	PiuContentUnbind(it, application, view);
	if ((*self)->data) {
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->reference);
		xsVar(1) = xsReference((*self)->data);
		xsCall1(xsVar(0), xsID_unload, xsVar(1));
		(*self)->frameCount = 0;
		(*self)->dataHeight= 0;
		(*self)->dataWidth = 0;
		(*self)->data = NULL;
		xsEndHost((*self)->the);
	}
}

void PiuSVGImage_create(xsMachine* the)
{
	PiuSVGImage* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuSVGImageRecord));
	self = PIU(SVGImage, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuSVGImageHooks);
	(*self)->dispatch = (PiuDispatch)&PiuSVGImageDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuSVGImageRecord));
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuSVGImageDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuSVGImage_get_opacity(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->o);
}

void PiuSVGImage_set_opacity(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->o = xsToNumber(xsArg(0));
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_center(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->cx = (c > 0) ? xsToNumber(xsArg(0)) : (xsNumberValue)(*self)->dataWidth / 2.0;
	(*self)->cy = (c > 1) ? xsToNumber(xsArg(1)) : (xsNumberValue)(*self)->dataHeight / 2.0;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_cx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->cx);
}

void PiuSVGImage_set_cx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->cx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_cy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->cy);
}

void PiuSVGImage_set_cy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->cy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_rotate(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->r = (c > 0) ? xsToNumber(xsArg(0)) : 0;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_r(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->r);
}

void PiuSVGImage_set_r(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->r = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_scale(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->sx = (c > 0) ? xsToNumber(xsArg(0)) : 1;
	(*self)->sy = (c > 1) ? xsToNumber(xsArg(1)) : (*self)->sx;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_s(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber(((*self)->sx + (*self)->sy) / 2);
}

void PiuSVGImage_set_s(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sx = (*self)->sy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_sx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->sx);
}

void PiuSVGImage_set_sx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_sy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->sy);
}

void PiuSVGImage_set_sy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_translate(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->tx = (c > 0) ? xsToNumber(xsArg(0)) : 0;
	(*self)->ty = (c > 1) ? xsToNumber(xsArg(1)) : (*self)->tx;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_tx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->tx);
}

void PiuSVGImage_set_tx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->tx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_ty(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->ty);
}

void PiuSVGImage_set_ty(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->ty = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}


