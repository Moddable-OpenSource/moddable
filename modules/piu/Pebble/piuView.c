/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

static void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuViewHooks = {
	PiuViewDelete,
	PiuViewMark,
	NULL
};

static EventServiceInfo	eventServiceDown;
static EventServiceInfo	eventServiceUp;

void PiuViewAdjust(PiuView* self) 
{
}

void PiuViewDelete(void* it)
{
	event_service_client_unsubscribe(&eventServiceUp);
	event_service_client_unsubscribe(&eventServiceDown);
}

void PiuViewDictionary(xsMachine* the, void* it)
{
}

void PiuViewDrawContent(PiuView* self, PiuViewDrawContentProc proc, void* it, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	(*proc)(it, self, x, y, sw, sh);
}

void PiuViewDrawString(PiuView* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension width, PiuDimension stringWidth)
{
	if ((*self)->color.a == 0) return;
	xsMachine* the = (*self)->the;
	xsStringValue text = PiuToString(string);
	GContext *ctx = (*self)->ctx;
	if ((*font)->gfont) {
		char tmp[100];
		text += offset;
		if ((-1 != length) && text[length]) {
			if (length >= (xsIntegerValue)sizeof(tmp))
				length = sizeof(tmp) - 1;
			c_memmove(tmp, text, length);
			tmp[length] = 0;
			text = tmp;
		}
		
		GRect box = GRect(
				x,
				y - fonts_get_font_cap_offset((*font)->gfont) + 1,	// +1 for leading applied by modFindPebbleFont
				stringWidth ? stringWidth : 10000,
				127);

		GColor saveColor = ctx->draw_state.text_color; 
		ctx->draw_state.text_color.argb = (*self)->color.argb;
		graphics_draw_text(ctx, text, (*font)->gfont, box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, C_NULL);
		ctx->draw_state.text_color = saveColor;
	}
	else {
		static const char ellipsisUTF8[4] = {0xE2, 0x80, 0xA6, 0};		// 0x2026
		static const char *ellipsisFallback = "...";
		const char *ellipsis;
		PocoDimension ellipsisWidth;
		xsIntegerValue character = 0;
		CFEGlyph glyph;
		PiuCoordinate advance;
		
		CFESetFontData(gCFE, (*font)->buffer, (*font)->bufferSize);
		CFESetFontSize(gCFE, (*font)->size);
		
		if (width) {
			glyph = CFEGetGlyphFromUnicode(gCFE, 0x2026, 0);
			if (glyph) {
				ellipsisWidth = glyph->advance;
				ellipsis = (char *)ellipsisUTF8;
			}
			else {
				glyph = CFEGetGlyphFromUnicode(gCFE, '.', 0);
				if (glyph) {
					ellipsisWidth = 3 * glyph->advance;
					ellipsis = ellipsisFallback;
				}
				else
					ellipsisWidth = 0;
			}
		}
		else {
			ellipsis = NULL;
			ellipsisWidth = 0;
		}
	
		text += offset;
		while (length) {
			xsStringValue formerText = text;
	#if MODDEF_CFE_KERN
			xsIntegerValue formerCharacter = character;
	#endif
			text = fxUTF8Decode(text, &character);
			if (character <= 0)
				break;
			length -= text - formerText;
				
			glyph = CFEGetGlyphFromUnicode(gCFE, character, 1);
			if (!glyph) {
				character = '?';
				glyph = CFEGetGlyphFromUnicode(gCFE, character, 1);
				if (!glyph)
					continue;
			}
			
			advance = glyph->advance;
	#if MODDEF_CFE_KERN
			if (formerCharacter)
				advance += CFEGetKerningOffset(gCFE, formerCharacter, character);
	#endif
			if (ellipsisWidth && ((width - advance) <= ellipsisWidth)) {
				ellipsisWidth = 0;
				if (stringWidth > width) {
					length = 3;
					text = (xsStringValue)ellipsis;
	#if MODDEF_CFE_KERN
					character = formerCharacter;
	#endif
					continue;
				}
			}
			PiuViewDrawTextureAux(self, (*font)->texture, x + glyph->dx, y + glyph->dy, glyph->sx, glyph->sy, glyph->w, glyph->h);
			x += advance;
			width -= advance;
			stringWidth -= advance;
		}
	}
}

void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		x -= sx;
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw)
		sw = tw - sx;
	if (sy < 0) {
		y -= sy;
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th)
		sh = th - sy;
	if ((sw <= 0) || (sh <= 0)) return;
	PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, sh);
}

void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	GContext *ctx = (*self)->ctx;
	GRect saveClip = ctx->draw_state.clip_box;
	GCompOp saveMode = ctx->draw_state.compositing_mode;
	GRect rect = GRect(x - sx, y - sy, sw + sx, sh + sy);
	grect_clip(&ctx->draw_state.clip_box, &GRect(x + ctx->draw_state.drawing_box.origin.x, y + ctx->draw_state.drawing_box.origin.y, sw, sh));
	if ((*texture)->gbitmap) {
		graphics_context_set_compositing_mode(ctx, GCompOpSet);
		graphics_draw_bitmap_in_rect_processed(ctx, (*texture)->gbitmap, &GRect(x - sx, y - sy, sw + sx, sh + sy), C_NULL);
	}
	else {
		PiuFlags flags = (*texture)->flags;
		GBitmap* bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
		GBitmap* mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
		if (mask) {
			if (bits) {
				if (bits->info.format == GBitmapFormat1Bit) {
					ctx->draw_state.compositing_mode = GCompOpSet;
					graphics_draw_bitmap_in_rect_processed(ctx, mask, &rect, C_NULL);
					ctx->draw_state.compositing_mode = GCompOpAnd;
				}
				else
					graphics_context_set_compositing_mode(ctx, GCompOpSet);
				graphics_draw_bitmap_in_rect_processed(ctx, bits, &rect, C_NULL);
			}
			else {
				#if PBL_COLOR
					GColor saveColor = ctx->draw_state.tint_color; 
					ctx->draw_state.tint_color.argb = (*self)->color.argb;
					ctx->draw_state.compositing_mode = GCompOpTint;
					graphics_draw_bitmap_in_rect_processed(ctx, mask, &rect, C_NULL);
					ctx->draw_state.tint_color = saveColor;
				#elif PBL_BW
					ctx->draw_state.compositing_mode = ((*self)->color.argb == GColorBlack.argb) ?  GCompOpAnd : GCompOpSet;
					graphics_draw_bitmap_in_rect_processed(ctx, mask, &rect, C_NULL);
				#else
					#error PBL_COLOR or PBL_BW expected
				#endif
			}
		}
		else {
			if (bits->info.format == GBitmapFormat1Bit)
				graphics_context_set_compositing_mode(ctx, GCompOpAssign);
			else
				graphics_context_set_compositing_mode(ctx, GCompOpSet);
			graphics_draw_bitmap_in_rect_processed(ctx, bits, &rect, C_NULL);
		}
	}
	ctx->draw_state.compositing_mode = saveMode;
	ctx->draw_state.clip_box = saveClip;
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if ((w <= 0) || (h <= 0)) return;
	if ((*self)->color.a == 0) return;
	GContext *ctx = (*self)->ctx;
	GRect r = GRect(x, y, w, h);
	GColor saveColor = ctx->draw_state.fill_color; 
	ctx->draw_state.fill_color.argb = (*self)->color.argb;
	graphics_fill_rect(ctx, &r);
	ctx->draw_state.fill_color = saveColor;
}

void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		if (w == sw) {
			x -= sx;
			w += sx;
		}
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw) {
		if (w == sw)
			w = tw - sx;
		sw = tw - sx;
	}
	if (sy < 0) {
		if (h == sh) {
			y -= sy;
			h += sy;
		}
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th) {
		if (h == sh)
			h = th - sy;
		sh = th - sy;
	}
	if ((w <= 0) || (h <= 0) || (sw <= 0) || (sh <= 0)) return;
	PiuCoordinate xx, ww;
	while (h >= sh) {
		xx = x;
		ww = w;
		while (ww >= sw) {
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, sw, sh);
			xx += sw;
			ww -= sw;
		}
		if (ww)
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, ww, sh);
		y += sh;
		h -= sh;
	}
	if (h) {
		while (w >= sw) {
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, h);
			x += sw;
			w -= sw;
		}
		if (w)
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, w, h);
	}
}

void PiuViewGetSize(PiuView* self, PiuDimension *width, PiuDimension *height)
{
//	GContext *ctx = app_state_get_graphics_context();
//	*width = ctx->dest_bitmap.bounds.size.w;
//	*height = ctx->dest_bitmap.bounds.size.h;
	Layer *layer = window_get_root_layer((*self)->window);
	GRect unobstructed_bounds;
	layer_get_unobstructed_bounds(layer, &unobstructed_bounds);
	*width = unobstructed_bounds.size.w;
	*height = unobstructed_bounds.size.h;
}

void PiuViewIdleCheck(PiuView* self, PiuInterval idle)
{
	xsMachine *the = (*self)->the;
	if (idle == (*self)->idle)
		return;
	(*self)->idle = idle;
	if (idle) {
		xsCall1(xsReference((*self)->screen), xsID_start, xsNumber(idle));
	}
	else {
		xsCall0(xsReference((*self)->screen), xsID_stop);
	}
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	Layer *layer = window_get_root_layer((*self)->window);
	layer_mark_dirty(layer);
}

void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuView self = it;
	PiuMarkReference(the, self->screen);
}

void PiuViewPopClip(PiuView* self)
{
	GContext *ctx = (*self)->ctx;
	PiuRectangle clip;
	if (0 == (*self)->clipDepth) {
		fxReport((*self)->the, "Piu clip stack underflow");
		return;
	}
	clip = &((*self)->clips[--((*self)->clipDepth)]);
	ctx->draw_state.clip_box = GRect(clip->x, clip->y, clip->width, clip->height);
}

void PiuViewPopColor(PiuView* self)
{
}

void PiuViewPopColorFilter(PiuView* self)
{
}

void PiuViewPopOrigin(PiuView* self)
{
	GContext *ctx = (*self)->ctx;
	PiuPoint origin;
	if (0 == (*self)->originDepth) {
		fxReport((*self)->the, "Piu origin stack underflow");
		return;
	}
	origin = &((*self)->origins[--((*self)->originDepth)]);
	ctx->draw_state.drawing_box.origin.x = origin->x;
	ctx->draw_state.drawing_box.origin.y = origin->y;
}

void PiuViewPushClip(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	GContext *ctx = (*self)->ctx;
	PiuRectangle clip;
	if ((MODDEF_PIU_PEBBLE_CLIPS - 1) == (*self)->clipDepth) {
		fxReport((*self)->the, "Piu clip stack overflow");
		return;
	}
	clip = &((*self)->clips[((*self)->clipDepth)++]);
	clip->x = ctx->draw_state.clip_box.origin.x;
	clip->y = ctx->draw_state.clip_box.origin.y;
	clip->width = ctx->draw_state.clip_box.size.w;
	clip->height = ctx->draw_state.clip_box.size.h;
	x += ctx->draw_state.drawing_box.origin.x;
	y += ctx->draw_state.drawing_box.origin.y;
	grect_clip(&ctx->draw_state.clip_box, &GRect(x, y, w, h));
}

void PiuViewPushColor(PiuView* self, PiuColor color)
{
	(*self)->color = GColorFromRGBA(color->r, color->g, color->b, color->a);
}

void PiuViewPushColorFilter(PiuView* self, PiuColor color)
{
	(*self)->color = GColorFromRGBA(color->r, color->g, color->b, color->a);
}

void PiuViewPushOrigin(PiuView* self, PiuCoordinate x, PiuCoordinate y)
{
	GContext *ctx = (*self)->ctx;
	PiuPoint origin;
	if ((MODDEF_PIU_PEBBLE_ORIGINS - 1) == (*self)->originDepth) {
		fxReport((*self)->the, "Piu origin stack overflow");
		return;
	}
	origin = &((*self)->origins[((*self)->originDepth)++]);
	origin->x = ctx->draw_state.drawing_box.origin.x;
	origin->y = ctx->draw_state.drawing_box.origin.y;
	ctx->draw_state.drawing_box.origin.x += x;
	ctx->draw_state.drawing_box.origin.y += y;
}

void PiuViewReflow(PiuView* self)
{
	Layer *layer = window_get_root_layer((*self)->window);
	layer_mark_dirty(layer);
}

void PiuViewReschedule(PiuView* self)
{
	PiuApplicationIdleCheck((*self)->application);
}

PiuTick PiuViewTicks(PiuView* self)
{
// 	if ((*self)->idleTicks)
// 		return (*self)->idleTicks;
	return modMilliseconds();
}

void PiuViewValidate(PiuView* self, PiuRectangle area) 
{
}

static void doUpdate(Layer *layer, GContext *ctx)
{
	Window *window = layer_get_window(layer);
	PiuView* self = window_get_user_data(window);
	PiuApplication* application = (*self)->application;
	PiuApplicationAdjust(application);

	PiuRectangleRecord area;
	PiuRectangleSet(&area, 0, 0, ctx->dest_bitmap.bounds.size.w, ctx->dest_bitmap.bounds.size.h);
	(*self)->ctx = ctx;
	(*(*application)->dispatch->update)(application, self, &area);
}

void PiuView_create(xsMachine* the) 
{
	PiuView* self;
	PiuApplication* application;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuViewRecord));
	self = PIU(View, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, &PiuViewHooks);
	application = (*self)->application = PIU(Application, xsArg(0));
	(*application)->view = self;
	(*self)->screen = xsToReference(xsArg(2));
	
	Window *window = app_window_stack_get_top_window();
	window_set_user_data(window, self);
	Layer *layer = window_get_root_layer(window);
	layer_set_update_proc(layer, doUpdate);
	(*self)->window = window;
}

void PiuView_onButton(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	PiuContent* content = (*application)->focus;
	xsVars(3);
	xsBooleanValue state = xsToBoolean(xsArg(0));
	xsStringValue which = xsToString(xsArg(1));
	xsIndex id;
	if (c_strcmp(which, "back") == 0)
		id = (state) ? xsID_onPressBack : xsID_onReleaseBack;
	else if (c_strcmp(which, "down") == 0)
		id = (state) ? xsID_onPressDown : xsID_onReleaseDown;
	else if (c_strcmp(which, "select") == 0)
		id = (state) ? xsID_onPressSelect : xsID_onReleaseSelect;
	else if (c_strcmp(which, "up") == 0)
		id = (state) ? xsID_onPressUp : xsID_onReleaseUp;
	else
		xsUnknownError("unknown button");
	while (content) {
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), id)) {
				xsVar(1) = xsReference((*content)->reference);
				xsVar(2) = xsCallFunction1(xsResult, xsVar(0), xsVar(1));
				if (xsTest(xsVar(2)))
					break;
			}
		}
		content = (PiuContent*)(*content)->container;
	}
	if ((content == NULL) && (id == xsID_onPressBack))
		window_stack_pop_with_transition(app_state_get_window_stack(), NULL);
}

void PiuView_onDisplayReady(xsMachine* the)
{
}

void PiuView_onIdle(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	xsVars(2);
	if (!application) return;
// 	(*self)->updating = 1;
// 	(*self)->idleTicks = PiuViewTicks(self);
	PiuApplicationDeferContents(the, application);
	PiuApplicationIdleContents(application);
	PiuApplicationTouchIdle(application);
 	PiuApplicationAdjust(application);
// 	(*self)->updating = 0;
 	PiuApplicationIdleCheck(application);
// 	(*self)->idleTicks = 0;
}

void PiuView_onMessage(xsMachine* the)
{
}

void PiuView_onQuit(xsMachine* the)
{
}

void PiuView_onResize(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	xsVars(2);
	PiuApplicationResize(application);
	if ((*application)->behavior) {
		xsVar(0) = xsReference((*application)->behavior);
		if (xsFindResult(xsVar(0), xsID_onResize)) {
			xsVar(1) = xsReference((*application)->reference);
			xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsArg(0));
		}
	}
}

void PiuView_onTouchBegan(xsMachine* the)
{
	PiuView* self = PIU(View, xsThis);
	PiuApplication* application = (*self)->application;
	if (!application) return;
	xsIntegerValue index = xsToInteger(xsArg(0));
	PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
	xsNumberValue ticks = xsToNumber(xsArg(3));
//    (*self)->updating = 1;
//    (*self)->idleTicks = (PiuTick)ticks;
    PiuApplicationTouchBegan(application, index, x, y, ticks);
    PiuApplicationAdjust(application);
//    (*self)->updating = 0;
//    PiuViewUpdate(self, application);
    PiuApplicationIdleCheck(application);
//    (*self)->idleTicks = 0;
}

void PiuView_onTouchEnded(xsMachine* the)
{
    PiuView* self = PIU(View, xsThis);
    PiuApplication* application = (*self)->application;
    if (!application) return;
    xsIntegerValue index = xsToInteger(xsArg(0));
    PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
    PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
    xsNumberValue ticks = xsToNumber(xsArg(3));
//    (*self)->updating = 1;
//    (*self)->idleTicks = (PiuTick)ticks;
    PiuApplicationTouchEnded(application, index, x, y, ticks);
    PiuApplicationAdjust(application);
//    (*self)->updating = 0;
//    PiuViewUpdate(self, application);
    PiuApplicationIdleCheck(application);
//    (*self)->idleTicks = 0;
}

void PiuView_onTouchMoved(xsMachine* the)
{
    PiuView* self = PIU(View, xsThis);
    PiuApplication* application = (*self)->application;
    if (!application) return;
    xsIntegerValue index = xsToInteger(xsArg(0));
    PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
    PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
    xsNumberValue ticks = xsToNumber(xsArg(3));
    PiuApplicationTouchMoved(application, index, x, y, ticks);
}

void PiuView_get_rotation(xsMachine* the) 
{
	xsResult = xsInteger(0);
}

void PiuView_get_ticks(xsMachine* the) 
{
	xsResult = xsNumber(modMilliseconds());
}

void PiuApplication_postMessage(xsMachine* the) 
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	xsVars(1);
	xsVar(0) = xsReference((*view)->screen);
	if (xsFindResult(xsVar(0), xsID_postMessage)) {
		(void)xsCallFunction1(xsResult, xsVar(0), xsArg(0));
	}
}

void PiuApplication_purge(xsMachine* the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuStyleLinkNew(the);
	(*self)->styleList = PIU(StyleLink, xsResult);
	xsSet(xsGlobal, xsID_assetMap, xsNull);
	xsCollectGarbage();
}
