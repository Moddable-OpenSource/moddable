/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

 #include "piuPebble.h"

typedef struct PiuQRCodeStruct PiuQRCodeRecord, *PiuQRCode;
struct PiuQRCodeStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* string;
	xsSlot* buffer;
	PiuDimension size;
	int fillColor;
	int strokeColor;
};

static void PiuQRCodeBind(void* it, PiuApplication* application, PiuView* view);
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
	PiuContentBind(it, application, view);
}

void PiuQRCodeDictionary(xsMachine* the, void* it) 
{
	PiuQRCode* self = it;
	if (xsFindResult(xsArg(1), xsID_string)) {
		xsSlot* string = PiuString(xsResult);
		(*self)->string = string;
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
			(*self)->fillColor = GColorFromRGBA(color.r, color.g, color.b, color.a).argb;
			PiuColorsBlend((*skin)->data.color.stroke, state, &color);
			(*self)->fillColor = GColorFromRGBA(color.r, color.g, color.b, color.a).argb;
		}
		else {
			(*self)->fillColor = GColorFromRGBA(255, 255, 255, 255).argb;
			(*self)->strokeColor = GColorFromRGBA(0, 0, 0, 255).argb;
		}
		PiuViewDrawContent(view, PiuQRCodeDrawAux, it, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
	}
}

void PiuQRCodeDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuQRCode* self = it;
	GContext *ctx = (*view)->ctx;
	GRect rect = GRect( x, y, sw, sh );
	PiuDimension size = (*self)->size;
	GColor saveColor;
	GBitmap mask;
	
	xsBeginHost((*self)->the);
	xsVars(1);
	xsVar(0) = xsReference((*self)->buffer);

	saveColor = ctx->draw_state.fill_color;
	ctx->draw_state.fill_color.argb = (*self)->fillColor;
	graphics_fill_rect(ctx, &rect);
	ctx->draw_state.fill_color = saveColor;
	
	mask.addr = xsToArrayBuffer(xsVar(0));
	mask.info.version = GBITMAP_VERSION_1;
	mask.bounds = GRect(0, 0, size, size);
	mask.row_size_bytes = ((size + 31) >> 5) * 4;
	mask.info.format = GBitmapFormat1Bit;
	rect = GRect( x + (sw - size) / 2, y + (sh - size) / 2, size, size );
#if PBL_COLOR
	saveColor = ctx->draw_state.tint_color; 
	ctx->draw_state.tint_color.argb = (*self)->strokeColor;
	ctx->draw_state.compositing_mode = GCompOpTint;
	graphics_draw_bitmap_in_rect_processed(ctx, &mask, &rect, C_NULL);
	ctx->draw_state.tint_color = saveColor;
#elif PBL_BW
	ctx->draw_state.compositing_mode = ((*self)->strokeColor == GColorBlack.argb) ?  GCompOpAnd : GCompOpSet;
	graphics_draw_bitmap_in_rect_processed(ctx, &mask, &rect, C_NULL);
#else
	#error PBL_COLOR or PBL_BW expected
#endif
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
		PiuDimension width = (*self)->bounds.width;
		PiuDimension height = (*self)->bounds.height;
		xsBeginHost((*self)->the);
		xsVars(3);
		xsVar(0) = xsReference((*self)->reference);
		xsVar(1) = *((*self)->string);
		xsVar(2) = xsNewObject();
		xsSet(xsVar(2), xsID_input, xsVar(1));
		xsSet(xsVar(2), xsID_bitmap, xsInteger(32));
		xsSet(xsVar(2), xsID_fit, xsInteger((width < height) ? width : height));
		xsResult = xsCall1(xsVar(0), xsID_QRCodeBuffer, xsVar(2));
		(*self)->buffer = xsToReference(xsResult);
		(*self)->size = xsToInteger(xsGet(xsResult, xsID_size));
		xsEndHost((*self)->the);
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
	(*self)->recordSize = PiuRecordSize(sizeof(PiuQRCodeRecord));
	(*self)->flags = piuVisible;
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
	PiuContentReflow(self, piuSizeChanged);
}


