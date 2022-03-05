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

extern void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor);

typedef struct PiuRoundContentStruct PiuRoundContentRecord, *PiuRoundContent;
struct PiuRoundContentStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuDimension border;
	PiuDimension radius;
};

static void PiuRoundContentDictionary(xsMachine* the, void* it);
static void PiuRoundContentDraw(void* it, PiuView* view, PiuRectangle area);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuRoundContentDispatchRecord = {
	"RoundContent",
	PiuContentBind,
	PiuContentCascade,
	PiuRoundContentDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
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

const xsHostHooks ICACHE_FLASH_ATTR PiuRoundContentHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuRoundContentDictionary(xsMachine* the, void* it) 
{
	PiuRoundContent* self = it;
	xsNumberValue number;
	if (xsFindNumber(xsArg(1), xsID_border, &number))
		(*self)->border = (PiuDimension)number;
	if (xsFindNumber(xsArg(1), xsID_radius, &number))
		(*self)->radius = (PiuDimension)number;
}

void PiuRoundContentDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuRoundContent* self = it;
	PiuSkin* skin = (*self)->skin;
	if (skin) {
		PiuRectangleRecord bounds;
		PiuColorRecord fillColor;
		PiuColorRecord strokeColor;
		PiuState state = (*self)->state;
		if (state < 0) state = 0;
		else if (3 < state) state = 3;
		PiuColorsBlend((*skin)->data.color.fill, state, &fillColor);
		PiuColorsBlend((*skin)->data.color.stroke, state, &strokeColor);
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawRoundContent(view, 0, 0, bounds.width, bounds.height, (*self)->radius, (*self)->border, (*self)->variant, &fillColor, &strokeColor);
		PiuViewPopClip(view);
	}
}

void PiuRoundContent_create(xsMachine* the)
{
	PiuRoundContent* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuRoundContentRecord));
	self = PIU(RoundContent, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuRoundContentHooks);
	(*self)->dispatch = (PiuDispatch)&PiuRoundContentDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuRoundContentDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuRoundContent_get_border(xsMachine* the)
{
	PiuRoundContent* self = PIU(RoundContent, xsThis);
	xsResult = xsPiuDimension((*self)->border);
}

void PiuRoundContent_get_radius(xsMachine* the)
{
	PiuRoundContent* self = PIU(RoundContent, xsThis);
	xsResult = xsPiuDimension((*self)->radius);
}

void PiuRoundContent_set_border(xsMachine *the)
{
	PiuRoundContent* self = PIU(RoundContent, xsThis);
	(*self)->border = xsToPiuDimension(xsArg(0));
	PiuContentInvalidate(self, NULL);
}

void PiuRoundContent_set_radius(xsMachine *the)
{
	PiuRoundContent* self = PIU(RoundContent, xsThis);
	(*self)->radius = xsToPiuDimension(xsArg(0));
	PiuContentInvalidate(self, NULL);
}


