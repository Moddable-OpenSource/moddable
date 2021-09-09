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

typedef struct PiuQRCodeStruct PiuQRCodeRecord, *PiuQRCode;
struct PiuQRCodeStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* string;
	xsIntegerValue maxVersion;
	xsSlot* buffer;
	PiuDimension scale;
	PiuCoordinate dx;
	PiuCoordinate dy;
	uint8_t fillBlend;
	PocoColor fillColor;
	uint8_t strokeBlend;
	PocoColor strokeColor;
};

static void PiuQRCodeBind(void* it, PiuApplication* application, PiuView* view);
static void PiuQRCodeBuffer(PiuQRCode* self);
static void PiuQRCodeDictionary(xsMachine* the, void* it);
static void PiuQRCodeDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuQRCodeDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuQRCodeMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuQRCodePlace(void* it);
static void PiuQRCodeUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuQRCodeDispatchRecord = {
	"QRCode",
	PiuQRCodeBind,
	PiuContentCascade,
	PiuQRCodeDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
	PiuQRCodePlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuQRCodeUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuQRCodeHooks = {
	PiuContentDelete,
	PiuQRCodeMark,
	NULL
};

void PiuQRCodeBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuQRCode* self = it;
	PiuContentBind(it, application, view);
	if ((*self)->string)
		PiuQRCodeBuffer(self);
}

void PiuQRCodeBuffer(PiuQRCode* self)
{
	xsBeginHost((*self)->the);
	xsVars(3);
	xsVar(0) = xsReference((*self)->reference);
	xsVar(1) = *((*self)->string);
	xsVar(2) = xsNewObject();
	xsSet(xsVar(2), xsID_input, xsVar(1));
	xsSet(xsVar(2), xsID_maxVersion, xsInteger((*self)->maxVersion));
	xsResult = xsCall1(xsVar(0), xsID_QRCodeBuffer, xsVar(2));
	(*self)->buffer = xsToReference(xsResult);
	xsEndHost((*self)->the);
}

void PiuQRCodeDictionary(xsMachine* the, void* it) 
{
	PiuQRCode* self = it;
	xsIntegerValue integer;
	if (xsFindResult(xsArg(1), xsID_string)) {
		xsSlot* string = PiuString(xsResult);
		(*self)->string = string;
	}
	if (xsFindInteger(xsArg(1), xsID_maxVersion, &integer)) {
		(*self)->maxVersion = integer;
	}
}

void PiuQRCodeDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuQRCode* self = it;
	if ((*self)->buffer) {
		PiuSkin* skin = (*self)->skin;
		if (skin) {
			PiuColorRecord color;
			PiuState state = (*self)->state;
			if (state < 0) state = 0;
			else if (3 < state) state = 3;
			PiuColorsBlend((*skin)->data.color.fill, state, &color);
			(*self)->fillColor = PocoMakeColor((*view)->poco, color.r, color.g, color.b);
			(*self)->fillBlend = color.a;
			PiuColorsBlend((*skin)->data.color.stroke, state, &color);
			(*self)->strokeColor = PocoMakeColor((*view)->poco, color.r, color.g, color.b);
			(*self)->strokeBlend = color.a;
		}
		else {
			(*self)->fillColor = PocoMakeColor((*view)->poco, 255, 255, 255);
			(*self)->fillBlend = 255;
			(*self)->strokeColor = PocoMakeColor((*view)->poco, 0, 0, 0);
			(*self)->strokeBlend = 255;
		}
		PiuViewDrawContent(view, PiuQRCodeDrawAux, it, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
	}
}

void PiuQRCodeDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuQRCode* self = it;
	xsBeginHost((*self)->the);
	xsVars(6);
	xsVar(0) = xsReference((*view)->reference);
	xsVar(0) = xsGet(xsVar(0), xsID_poco);
	if ((*self)->fillBlend) {
		xsVar(1) = xsInteger((*self)->fillColor);
		xsVar(2) = xsInteger(x);
		xsVar(3) = xsInteger(y);
		xsVar(4) = xsInteger(sw);
		xsVar(5) = xsInteger(sh);
		xsCall5(xsVar(0), xsID_fillRectangle, xsVar(1), xsVar(2), xsVar(3), xsVar(4), xsVar(5));
	}
	if ((*self)->strokeBlend) {
		xsVar(1) = xsReference((*self)->buffer);
		xsVar(2) = xsInteger(x + (*self)->dx);
		xsVar(3) = xsInteger(y + (*self)->dy);
		xsVar(4) = xsInteger((*self)->scale);
		xsVar(5) = xsInteger((*self)->strokeColor);
		xsCall5(xsVar(0), xsID_drawQRCode, xsVar(1), xsVar(2), xsVar(3), xsVar(4), xsVar(5));
	}
	xsEndHost((*self)->the);
}

void PiuQRCodeMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuQRCode self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkString(the, self->string);
	PiuMarkReference(the, self->buffer);
}

void PiuQRCodePlace(void* it) 
{
	PiuQRCode* self = it;
	if ((*self)->flags & piuPlaced) {
		if ((*self)->buffer) {
			PiuDimension width = (*self)->bounds.width;
			PiuDimension height = (*self)->bounds.height;
			PiuDimension size, scale;
			xsBeginHost((*self)->the);
			xsVars(1);
			xsVar(0) = xsReference((*self)->buffer);
			size = xsToInteger(xsGet(xsVar(0), xsID_size));
			xsEndHost((*self)->the);
			scale = (width < height) ? width : height;
			scale /= size;
			if (scale < 1) scale = 1;
			(*self)->scale = scale;
			scale *= size;
			(*self)->dx = (width - scale) >> 1;
			(*self)->dy = (height - scale) >> 1;
		}
	}
	PiuContentPlace(it);
}

void PiuQRCodeUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuQRCode* self = it;
	(*self)->buffer = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuQRCode_create(xsMachine* the)
{
	PiuQRCode* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuQRCodeRecord));
	self = PIU(QRCode, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuQRCodeHooks);
	(*self)->dispatch = (PiuDispatch)&PiuQRCodeDispatchRecord;
	(*self)->flags = piuVisible;
	(*self)->maxVersion = 40;
	PiuContentDictionary(the, self);
	PiuQRCodeDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuQRCode_get_string(xsMachine* the)
{
	PiuQRCode* self = PIU(QRCode, xsThis);
	if ((*self)->string)
		xsResult = *((*self)->string);
}

void PiuQRCode_set_string(xsMachine *the)
{
	PiuQRCode* self = PIU(QRCode, xsThis);
	xsSlot* string = PiuString(xsArg(0));
	(*self)->string = string;
	if ((*self)->application)
		PiuQRCodeBuffer(self);
	PiuContentReflow(self, piuSizeChanged);
}


