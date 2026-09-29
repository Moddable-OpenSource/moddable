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

typedef struct PiuRoundRectStruct PiuRoundRectRecord, *PiuRoundRect;
struct PiuRoundRectStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsIntegerValue corners;
	xsIntegerValue radius;
	uint8_t fillBlend;
	PocoColor fillColor;
	void* fillOutline;
};

static void PiuRoundRectDictionary(xsMachine* the, void* it);
static void PiuRoundRectDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuRoundRectDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuRoundRectMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuRoundRectPlace(void* it);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuRoundRectDispatchRecord = {
	"RoundRect",
	PiuContentBind,
	PiuContentCascade,
	PiuRoundRectDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
	PiuRoundRectPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuContentUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuRoundRectHooks = {
	PiuContentDelete,
	PiuRoundRectMark,
	NULL
};

void PiuRoundRectDictionary(xsMachine* the, void* it) 
{
	PiuRoundRect* self = it;
	xsIntegerValue integer;
	if (xsFindInteger(xsArg(1), xsID_corners, &integer)) {
		(*self)->corners = integer;
	}
	if (xsFindInteger(xsArg(1), xsID_radius, &integer)) {
		(*self)->radius = integer;
	}
}

void PiuRoundRectDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuRoundRect* self = it;
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
			PiuRectangleRecord bounds;
			PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
			PiuViewDrawContent(view, PiuRoundRectDrawAux, it, 0, 0, bounds.width, bounds.height);
		}
	}
}

void PiuRoundRectDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuRoundRect* self = it;
	xsBeginHost((*self)->the);
	xsVars(3);
	xsVar(0) = xsReference((*self)->fillOutline);
	PocoOutline outline = xsGetHostData(xsVar(0));
	PocoOutlineFill((*view)->poco, (*self)->fillColor, (*self)->fillBlend, outline, x, y);
	xsEndHost((*self)->the);
}

void PiuRoundRectMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuRoundRect self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkReference(the, self->fillOutline);
}

void PiuRoundRectPlace(void* it) 
{
	PiuRoundRect* self = it;
	xsBeginHost((*self)->the);
	xsThis = xsReference((*self)->reference);
	xsResult = xsCall4(xsThis, xsID_place, xsInteger((*self)->bounds.width), xsInteger((*self)->bounds.height), xsInteger((*self)->corners), xsInteger((*self)->radius));
	(*self)->fillOutline = xsToReference(xsResult);
	xsEndHost((*self)->the);
	PiuContentPlace(it);
}

void PiuRoundRect_create(xsMachine* the)
{
	PiuRoundRect* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuRoundRectRecord));
	self = PIU(RoundRect, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuRoundRectHooks);
	(*self)->dispatch = (PiuDispatch)&PiuRoundRectDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuRoundRectRecord));
	(*self)->flags = piuVisible;
	(*self)->corners = 0xF;
	(*self)->radius = 8;
	PiuContentDictionary(the, self);
	PiuRoundRectDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuRoundRect_get_corners(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	xsResult = xsInteger((*self)->corners);
}

void PiuRoundRect_set_corners(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	(*self)->corners = xsToInteger(xsArg(0));
	PiuContentReflow(self, piuSizeChanged);
}

void PiuRoundRect_get_radius(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	xsResult = xsInteger((*self)->radius);
}

void PiuRoundRect_set_radius(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	(*self)->radius = xsToInteger(xsArg(0));
	PiuContentReflow(self, piuSizeChanged);
}
