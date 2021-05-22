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

static void PiuPortBind(void* it, PiuApplication* application, PiuView* view);
static void PiuPortCascade(void* it);
static void PiuPortComputeStyle(PiuPort* self);
static void PiuPortDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuPortMeasureHorizontally(void* it);
static void PiuPortMeasureVertically(void* it);
static void PiuPortUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuPortDispatchRecord = {
	"Port",
	PiuPortBind,
	PiuPortCascade,
	PiuPortDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuPortMeasureHorizontally,
	PiuPortMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuPortUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuPortHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuPortBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuPort* self = it;
	PiuContentBind(it, application, view);
	PiuPortComputeStyle(self);
}

void PiuPortCascade(void* it)
{
	PiuPort* self = it;
	PiuContentCascade(it);
	PiuPortComputeStyle(self);
}

void PiuPortComputeStyle(PiuPort* self)
{
	xsMachine* the = (*self)->the;
	PiuApplication* application = (*self)->application;
	PiuContainer* container = (PiuContainer*)self;
	PiuStyleLink* list = (*application)->styleList;
	PiuStyleLink* chain = NULL;
	while (container) {
		PiuStyle* style = (*container)->style;
		if (style) {
			list = PiuStyleLinkMatch(the, list, chain, style);
			chain = list;
		}
		container = (*container)->container;
	}
	if (chain) {
		PiuStyle* result = PiuStyleLinkCompute(the, chain, application);
		(*self)->computedStyle = result;
	}
}

void PiuPortDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuPort* self = it;
	PiuContentDraw(it, view, area);
	(*self)->view = view;
	PiuBehaviorOnDraw(it, area);
	(*self)->view = NULL;
}

void PiuPortMeasureHorizontally(void* it) 
{
	PiuPort* self = it;
	PiuContentMeasureHorizontally(it);
	(*self)->coordinates.width = PiuBehaviorOnMeasureHorizontally(self, (*self)->coordinates.width);
}

void PiuPortMeasureVertically(void* it) 
{
	PiuPort* self = it;
	PiuContentMeasureVertically(it);
	(*self)->coordinates.height = PiuBehaviorOnMeasureVertically(self, (*self)->coordinates.height);
}

void PiuPortUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuPort* self = it;
	(*self)->computedStyle = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuPort_create(xsMachine* the)
{
	PiuPort* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuPortRecord));
	self = PIU(Port, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuPortHooks);
	(*self)->dispatch = (PiuDispatch)&PiuPortDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuPort_set_skin(xsMachine *the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuSkin* skin = NULL;
	if (xsTest(xsArg(0)))
		skin = PIU(Skin, xsArg(0));
	(*self)->skin = skin;
}

void PiuPort_set_state(xsMachine *the)
{
	PiuPort* self = PIU(Port, xsThis);
	(*self)->state = xsToNumber(xsArg(0));
}

void PiuPort_set_variant(xsMachine *the)
{
	PiuPort* self = PIU(Port, xsThis);
	(*self)->variant = (PiuVariant)xsToInteger(xsArg(0)); 
}

void PiuPort_drawContent(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuRectangleRecord bounds;
	if (!view) xsUnknownError("out of sequence");
	bounds.x = xsToPiuCoordinate(xsArg(0));
	bounds.y = xsToPiuCoordinate(xsArg(1));
	bounds.width = xsToPiuDimension(xsArg(2));
	bounds.height = xsToPiuDimension(xsArg(3));
	if ((*self)->skin)
		PiuSkinDraw((*self)->skin, view, &bounds, (*self)->variant, (*self)->state, piuCenter, piuMiddle);
}

void PiuPort_drawLabel(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	xsSlot* string;
	PiuRectangleRecord bounds;
	PiuBoolean ellipsis;
	if (!view) xsUnknownError("out of sequence");
	string = PiuString(xsArg(0));
	bounds.x = xsToPiuCoordinate(xsArg(1));
	bounds.y = xsToPiuCoordinate(xsArg(2));
	bounds.width = xsToPiuDimension(xsArg(3));
	bounds.height = xsToPiuDimension(xsArg(4));
	ellipsis = (c > 5) ? (PiuBoolean)xsToBoolean(xsArg(5)) : 0;
	if ((*self)->skin)
		PiuSkinDraw((*self)->skin, view, &bounds, (*self)->variant, (*self)->state, piuCenter, piuMiddle);
	if ((*self)->computedStyle)
		PiuStyleDraw((*self)->computedStyle, string, view, &bounds, ellipsis, (*self)->state);
}

void PiuPort_drawSkin(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuSkin* skin;
	PiuRectangleRecord bounds;
	PiuVariant variant;
	PiuState state;
	if (!view) xsUnknownError("out of sequence");
	skin = PIU(Skin, xsArg(0));
	bounds.x = xsToPiuCoordinate(xsArg(1));
	bounds.y = xsToPiuCoordinate(xsArg(2));
	bounds.width = xsToPiuDimension(xsArg(3));
	bounds.height = xsToPiuDimension(xsArg(4));
	variant = (c > 5) ? (PiuVariant)xsToInteger(xsArg(5)) : 0;
	state = (c > 6) ? (PiuState)xsToNumber(xsArg(6)) : 0;
	PiuSkinDraw(skin, view, &bounds, variant, state, piuCenter, piuMiddle);
}

void PiuPort_drawString(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	xsSlot* string;
	PiuStyle* style;
	PiuColorRecord color;
	PiuCoordinate x, y;
	PiuDimension w, sw;
	if (!view) xsUnknownError("out of sequence");
	style = PIU(Style, xsArg(1));
	if ((*style)->font == NULL)
		PiuStyleLookupFont(style);
	PiuColorDictionary(the, &xsArg(2), &color);
	string = PiuString(xsArg(0));
	x = xsToPiuCoordinate(xsArg(3));
	y = xsToPiuCoordinate(xsArg(4));
	w = (c > 5) ? xsToPiuDimension(xsArg(5)) : 0;
	sw = w ? PiuFontGetWidth((*style)->font, string, 0, -1) : 0;
	PiuViewPushColor(view, &color);
	PiuViewDrawString(view, string, 0, -1, (*style)->font, x, y, w, sw);
	PiuViewPopColor(view);
}

void PiuPort_drawStyle(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	xsSlot* string;
	PiuStyle* style;
	PiuRectangleRecord bounds;
	PiuBoolean ellipsis;
	PiuState state;
	if (!view) xsUnknownError("out of sequence");
	style = PIU(Style, xsArg(1));
	if ((*style)->font == NULL)
		PiuStyleLookupFont(style);
	string = PiuString(xsArg(0));
	bounds.x = xsToPiuCoordinate(xsArg(2));
	bounds.y = xsToPiuCoordinate(xsArg(3));
	bounds.width = xsToPiuDimension(xsArg(4));
	bounds.height = xsToPiuDimension(xsArg(5));
	ellipsis = (c > 6) ? (PiuBoolean)xsToBoolean(xsArg(6)) : 0;
	state = (c > 7) ? (PiuState)xsToNumber(xsArg(7)) : 0;
	PiuStyleDraw(style, string, view, &bounds, ellipsis, state);
}

void PiuPort_drawTexture(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuTexture* texture;
	PiuColorRecord color;
	PiuCoordinate x, y, sx, sy;
	PiuDimension sw, sh;
	if (!view) xsUnknownError("out of sequence");
	texture = PIU(Texture, xsArg(0));
	PiuColorDictionary(the, &xsArg(1), &color);
	x = xsToPiuCoordinate(xsArg(2));
	y = xsToPiuCoordinate(xsArg(3));
	sx = xsToPiuCoordinate(xsArg(4));
	sy = xsToPiuCoordinate(xsArg(5));
	sw = xsToPiuDimension(xsArg(6));
	sh = xsToPiuDimension(xsArg(7));
	PiuViewPushColor(view, &color);
	PiuViewDrawTexture(view, texture, x, y, sx, sy, sw, sh);
	PiuViewPopColor(view);
}

void PiuPort_fillColor(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuColorRecord color;
	PiuCoordinate x, y;
	PiuDimension w, h;
	if (!view) xsUnknownError("out of sequence");
	PiuColorDictionary(the, &xsArg(0), &color);
	x = xsToPiuCoordinate(xsArg(1));
	y = xsToPiuCoordinate(xsArg(2));
	w = xsToPiuDimension(xsArg(3));
	h = xsToPiuDimension(xsArg(4));
	PiuViewPushColor(view, &color);
	PiuViewFillColor(view, x, y, w, h);
	PiuViewPopColor(view);
}

void PiuPort_fillTexture(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuTexture* texture;
	PiuColorRecord color;
	PiuCoordinate x, y, sx, sy;
	PiuDimension w, h, sw, sh;
	if (!view) xsUnknownError("out of sequence");
	texture = PIU(Texture, xsArg(0));
	PiuColorDictionary(the, &xsArg(1), &color);
	x = xsToPiuCoordinate(xsArg(2));
	y = xsToPiuCoordinate(xsArg(3));
	w = xsToPiuDimension(xsArg(4));
	h = xsToPiuDimension(xsArg(5));
	sx = xsToPiuCoordinate(xsArg(6));
	sy = xsToPiuCoordinate(xsArg(7));
	sw = xsToPiuDimension(xsArg(8));
	sh = xsToPiuDimension(xsArg(9));
	PiuViewPushColor(view, &color);
	PiuViewFillTexture(view, texture, x, y, w, h, sx, sy, sw, sh);
	PiuViewPopColor(view);
}

void PiuPort_invalidate(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	if (view) xsUnknownError("out of sequence");
	if (c >= 4) {
		PiuRectangleRecord area;
		PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
		PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
		PiuDimension w = xsToPiuDimension(xsArg(2));
		PiuDimension h = xsToPiuDimension(xsArg(3));
		PiuRectangleSet(&area, x, y, w, h);
		PiuContentInvalidate(self, &area);
	}
	else
		PiuContentInvalidate(self, NULL);
}

void PiuPort_popClip(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	if (!view) xsUnknownError("out of sequence");
	PiuViewPopClip(view);
}

void PiuPort_pushClip(xsMachine* the)
{
	PiuPort* self = PIU(Port, xsThis);
	PiuView* view = (*self)->view;
	PiuCoordinate x, y;
	PiuDimension w, h;
	if (!view) xsUnknownError("out of sequence");
	x = xsToPiuCoordinate(xsArg(0));
	y = xsToPiuCoordinate(xsArg(1));
	w = xsToPiuDimension(xsArg(2));
	h = xsToPiuDimension(xsArg(3));
	PiuViewPushClip(view, x, y, w, h);
}

void PiuPort_measureString(xsMachine* the)
{
	xsSlot* string;
	PiuStyle* style;
	PiuDimension w, h;
	(void)xsToString(xsArg(0));
	string = &xsArg(0);
	style = PIU(Style, xsArg(1));
	if ((*style)->font == NULL)
		PiuStyleLookupFont(style);
	w = PiuFontGetWidth((*style)->font, string, 0, -1);
	h = PiuFontGetHeight((*style)->font);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_width, xsPiuDimension(w), xsDefault);
	xsDefine(xsResult, xsID_height, xsPiuDimension(h), xsDefault);
}
