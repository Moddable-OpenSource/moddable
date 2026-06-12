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

typedef struct PiuScreenBufferStruct PiuScreenBufferRecord, *PiuScreenBuffer;
struct PiuScreenBufferStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	GBitmap* buffer;
};

static void PiuScreenBufferBind(void* it, PiuApplication* application, PiuView* view);
static void PiuScreenBufferDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuScreenBufferDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuScreenBufferUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuScreenBufferDispatchRecord = {
	"ScreenBuffer",
	PiuScreenBufferBind,
	PiuContentCascade,
	PiuScreenBufferDraw,
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
	PiuScreenBufferUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuScreenBufferHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuScreenBufferBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreenBuffer* self = it;
	GContext *ctx = (*view)->ctx;
	GBitmap* context_bitmap = graphics_context_get_bitmap(ctx);
	GBitmap* buffer_bitmap = gbitmap_create_blank(context_bitmap->bounds.size, context_bitmap->info.format);
	bitblt_bitmap_into_bitmap(buffer_bitmap, context_bitmap, GPointZero, GCompOpAssign, GColorWhite);
	(*self)->buffer = buffer_bitmap;
	PiuContentBind(it, application, view);
}

void PiuScreenBufferDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuScreenBuffer* self = it;
	PiuRectangleRecord bounds;
	PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
	PiuViewDrawContent(view, PiuScreenBufferDrawAux, it, 0, 0, bounds.width, bounds.height);
}

void PiuScreenBufferDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuScreenBuffer* self = it;
	GContext *ctx = (*view)->ctx;
	GBitmap* sub_bitmap = (*self)->buffer;
	x += ctx->draw_state.drawing_box.origin.x;
	y += ctx->draw_state.drawing_box.origin.y;
	GRect box = GRect(x, y, sw, sh);
	GCompOp saveMode = ctx->draw_state.compositing_mode;
	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	graphics_draw_bitmap_in_rect(ctx, sub_bitmap, &box);
	ctx->draw_state.compositing_mode = saveMode;
}

void PiuScreenBufferUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreenBuffer* self = it;
	PiuContentUnbind(it, application, view);
	if ((*self)->buffer) {
		gbitmap_destroy((*self)->buffer);
		(*self)->buffer = NULL;
	}
}

void PiuScreenBuffer_create(xsMachine* the)
{
	PiuScreenBuffer* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuScreenBufferRecord));
	self = PIU(ScreenBuffer, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuScreenBufferHooks);
	(*self)->dispatch = (PiuDispatch)&PiuScreenBufferDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuScreenBufferRecord));
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuBehaviorOnCreate(self);
}


