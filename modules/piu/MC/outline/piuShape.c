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
#include "freetype.h"
#include "ftobjs.h"
#include "ftstroke.h"
#include "commodettoPocoOutline.h"

typedef struct PiuShapeStruct PiuShapeRecord, *PiuShape;
struct PiuShapeStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	
	uint8_t fillBlend;
	PocoColor fillColor;
	void* fillOutline;

	uint8_t strokeBlend;
	PocoColor strokeColor;
	void* strokeOutline;
};

static void PiuShapeBind(void* it, PiuApplication* application, PiuView* view);
static void PiuShapeDictionary(xsMachine* the, void* it);
static void PiuShapeDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuShapeDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuShapeMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuShapeMeasureHorizontally(void* it);
static void PiuShapeMeasureVertically(void* it);
static void PiuShapeUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuShapeDispatchRecord = {
	"Shape",
	PiuShapeBind,
	PiuContentCascade,
	PiuShapeDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuShapeMeasureHorizontally,
	PiuShapeMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuShapeUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuShapeHooks = {
	PiuContentDelete,
	PiuShapeMark,
	NULL
};

void PiuShapeBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContentBind(it, application, view);
}

void PiuShapeDictionary(xsMachine* the, void* it) 
{
}

void PiuShapeDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuShape* self = it;
	PiuSkin* skin = (*self)->skin;
	if (skin) {
		PiuColorRecord color;
		PiuState state = (*self)->state;
		if (state < 0) state = 0;
		else if (3 < state) state = 3;
		if ((*self)->fillOutline) {
			PiuColorsBlend((*skin)->data.color.fill, state, &color);
			(*self)->fillColor = PocoMakeColor((*view)->poco, color.r, color.g, color.b);
			(*self)->fillBlend = color.a;
		}
		if ((*self)->strokeOutline) {
			PiuColorsBlend((*skin)->data.color.stroke, state, &color);
			(*self)->strokeColor = PocoMakeColor((*view)->poco, color.r, color.g, color.b);
			(*self)->strokeBlend = color.a;
		}
		if ((*self)->fillOutline || (*self)->strokeOutline) {
			PiuRectangleRecord bounds;
			PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
			PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
			PiuViewDrawContent(view, PiuShapeDrawAux, it, 0, 0, bounds.width, bounds.height);
			PiuViewPopClip(view);
		}
	}
}

void PiuShapeDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuShape* self = it;
	PocoOutline outline;
	xsBeginHost((*self)->the);
	if ((*self)->fillOutline) {
		xsResult = xsReference((*self)->fillOutline);
		outline = xsGetHostData(xsResult);
		PocoOutlineFill((*view)->poco, (*self)->fillColor, (*self)->fillBlend, outline, x, y);
	}
	if ((*self)->strokeOutline) {
		xsResult = xsReference((*self)->strokeOutline);
		outline = xsGetHostData(xsResult);
		PocoOutlineFill((*view)->poco, (*self)->strokeColor, (*self)->strokeBlend, outline, x, y);
	}
	xsEndHost((*self)->the);
}

void PiuShapeMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuShape self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkReference(the, self->fillOutline);
	PiuMarkReference(the, self->strokeOutline);
}

void PiuShapeMeasureHorizontally(void* it) 
{
	PiuShape* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth)) {
		PiuDimension fillWidth = 0;
		PiuDimension strokeWidth = 0;
		PocoOutline outline;
		xsBeginHost((*self)->the);
		if ((*self)->fillOutline) {
			xsResult = xsReference((*self)->fillOutline);
			outline = xsGetHostData(xsResult);
		#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
			PocoOutlineUnrotate(outline);
		#endif
			PocoOutlineCalculateCBox(outline);
			fillWidth = outline->w;
		}
		if ((*self)->strokeOutline) {
			xsResult = xsReference((*self)->strokeOutline);
            outline = xsGetHostData(xsResult);
		#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
			PocoOutlineUnrotate(outline);
		#endif
			PocoOutlineCalculateCBox(outline);
			strokeWidth = outline->w;
		}
		xsEndHost((*self)->the);
		(*self)->coordinates.width = (fillWidth > strokeWidth) ? fillWidth : strokeWidth;
	}
}

void PiuShapeMeasureVertically(void* it) 
{
	PiuShape* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight)) {
		PiuDimension fillHeight = 0;
		PiuDimension strokeHeight = 0;
		PocoOutline outline;
		xsBeginHost((*self)->the);
		if ((*self)->fillOutline) {
			xsResult = xsReference((*self)->fillOutline);
			outline = xsGetHostData(xsResult);
		#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
			PocoOutlineUnrotate(outline);
		#endif
			PocoOutlineCalculateCBox(outline);
			fillHeight = outline->h;
		}
		if ((*self)->strokeOutline) {
			xsResult = xsReference((*self)->strokeOutline);
			outline = xsGetHostData(xsResult);
		#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
			PocoOutlineUnrotate(outline);
		#endif
			PocoOutlineCalculateCBox(outline);
			strokeHeight = outline->h;
		}
		xsEndHost((*self)->the);
		(*self)->coordinates.height = (fillHeight > strokeHeight) ? fillHeight : strokeHeight;
	}
}

void PiuShapeUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContentUnbind(it, application, view);
}

void PiuShape_create(xsMachine* the)
{
	PiuShape* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuShapeRecord));
	self = PIU(Shape, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuShapeHooks);
	(*self)->dispatch = (PiuDispatch)&PiuShapeDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuShapeDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuShape_get_fillOutline(xsMachine* the)
{
	PiuShape* self = PIU(Shape, xsThis);
	xsSlot* fillOutline = (*self)->fillOutline;
	if (fillOutline)
		xsResult = xsReference(fillOutline);
}

void PiuShape_get_strokeOutline(xsMachine* the)
{
	PiuShape* self = PIU(Shape, xsThis);
	xsSlot* strokeOutline = (*self)->strokeOutline;
	if (strokeOutline)
		xsResult = xsReference(strokeOutline);
}

void PiuShape_set_fillOutline(xsMachine *the)
{
	PiuShape* self = PIU(Shape, xsThis);
	(*self)->fillOutline = xsToReference(xsArg(0));
	PiuContentReflow(self, piuSizeChanged);
}

void PiuShape_set_strokeOutline(xsMachine *the)
{
	PiuShape* self = PIU(Shape, xsThis);
	(*self)->strokeOutline = xsToReference(xsArg(0));
	PiuContentReflow(self, piuSizeChanged);
}


