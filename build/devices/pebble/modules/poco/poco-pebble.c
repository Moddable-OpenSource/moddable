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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "commodettoBitmap.h"
#include "commodettoPoco.h"
#include "commodettoPocoBlit.h"

#include "applib/graphics/gcontext.h"
#include "applib/graphics/gdraw_command_image.h"
#include "applib/graphics/gdraw_command_sequence.h"
#include "applib/graphics/gdraw_command_transforms.h"
#include "applib/graphics/gtypes.h"
#include "applib/graphics/graphics.h"
#include "applib/graphics/graphics_line.h"
#include "util/trig.h"

/*
	ORIGIN
	CLIPPING
*/

extern GContext *getPocoGContext(xsMachine *the);

void xs_pocopebble_Font_destructor(void *data)
{
}

static void xs_pocopebble_Font_destructor_custom(void *data)
{
	if (data)
		fonts_unload_custom_font((GFont)data);
}

void xs_pocopebble_Font(xsMachine *the)
{
	GFont font;
	int32_t ascent, descent, leading;
	int type = xsmcTypeOf(xsArg(0));
	uint8_t isNumber = (xsNumberType == type) || (xsIntegerType == type);
	if (isNumber) {
		ResHandle resHandle = applib_resource_get_handle(xsmcToInteger(xsArg(0)));
		if (!resHandle)
			xsUnknownError("resource not found");
		font = fonts_load_custom_font(resHandle);
	}
	else {
		int size = xsmcToInteger(xsArg(1));
		const char *family = xsmcToString(xsArg(0));
		font = modFindPebbleFont(family, size, &ascent, &descent, &leading);
	}

	if (!font)
		xsUnknownError("font not found");

	xsmcSetHostData(xsThis, font);
	if (isNumber) {
		xsmcSetHostDestructor(xsThis, xs_pocopebble_Font_destructor_custom);

		// matches calculations in modFindPebbleFont
		uint16_t height = fonts_get_font_height(font);
		descent = fonts_get_font_cap_offset(font);
		leading = 1;
		ascent = height - descent - leading;
	}

	xsSlot tmp;
	xsmcSetInteger(tmp, ascent + descent + leading);
	xsmcDefine(xsThis, xsID_height, tmp, xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, ascent);
	xsmcDefine(xsThis, xsID_ascent, tmp, xsDontDelete | xsDontSet);
}

void xs_pocopebble_getTextWidth(xsMachine *the)
{
	const void *destructor =xsGetHostDestructor(xsArg(1));
	if ((xs_pocopebble_Font_destructor != destructor) && (xs_pocopebble_Font_destructor_custom != destructor)) {
		xs_poco_getTextWidth(the);
		return;
	}

	GFont font = xsmcGetHostData(xsArg(1));
	GContext *ctx = getPocoGContext(the);

	GSize size = graphics_text_layout_get_max_used_size(
				ctx, xsmcToString(xsArg(0)),
				font, GRect(0, 0, 10000, 100),
				GTextOverflowModeTrailingEllipsis,
				GTextAlignmentLeft, NULL);

	xsmcSetInteger(xsResult, size.w);
}

void xs_pocopebbble_drawText(xsMachine *the)
{
	const void *destructor =xsGetHostDestructor(xsArg(1));
	if ((xs_pocopebble_Font_destructor != destructor) && (xs_pocopebble_Font_destructor_custom != destructor)) {
		xs_poco_drawText(the);
		return;
	}

	GFont font = xsmcGetHostData(xsArg(1));
	GContext *ctx = getPocoGContext(the);

	GRect box = GRect(xsmcToInteger(
			xsArg(3)),
			xsmcToInteger(xsArg(4)) - fonts_get_font_cap_offset(font) + 1,	// +1 for leading applied by modFindPebbleFont
			10000, 127);

	GColor saveTextColor = ctx->draw_state.text_color; 
	ctx->draw_state.text_color.argb = xsmcToInteger(xsArg(2));
	graphics_draw_text(ctx, xsmcToString(xsArg(0)), font, box,
		GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, C_NULL);
	ctx->draw_state.text_color = saveTextColor;
}

void xs_pocopebbble_drawLine(xsMachine *the)
{
	GContext *ctx = getPocoGContext(the);
	GPoint from, to;
	from.x = xsmcToInteger(xsArg(0));
	from.y = xsmcToInteger(xsArg(1));
	to.x = xsmcToInteger(xsArg(2));
	to.y = xsmcToInteger(xsArg(3));
	int color = xsmcToInteger(xsArg(4));
	int width = (xsmcArgc > 5) ? xsmcToInteger(xsArg(5)) : 1;

	GColor saveStrokeColor = ctx->draw_state.text_color; 
	ctx->draw_state.stroke_color.argb = color;
	uint8_t saveStrokeWidth = ctx->draw_state.stroke_width;
	ctx->draw_state.stroke_width = (uint8_t)width;

	graphics_draw_line(ctx, from, to);

	ctx->draw_state.stroke_color = saveStrokeColor;
	ctx->draw_state.stroke_width = saveStrokeWidth;
}

void xs_pocopebbble_drawRoundRect(xsMachine *the)
{
	GContext *ctx = getPocoGContext(the);
	GRect r = {
		.origin.x = xsmcToInteger(xsArg(0)),
		.origin.y = xsmcToInteger(xsArg(1)),
		.size.w = xsmcToInteger(xsArg(2)),
		.size.h = xsmcToInteger(xsArg(3)),
	};
	int color = xsmcToInteger(xsArg(4));
	int radius = (xsmcArgc > 5) ? xsmcToInteger(xsArg(5)) : 4;
	int corners = (xsmcArgc > 6) ? xsmcToInteger(xsArg(6)) : GCornersAll;

	GColor saveColor = ctx->draw_state.fill_color;
	ctx->draw_state.fill_color.argb = color;
	graphics_fill_round_rect(ctx, &r, radius, corners);
	ctx->draw_state.fill_color = saveColor;
}

void xs_pocopebbble_frameRoundRect(xsMachine *the)
{
	GContext *ctx = getPocoGContext(the);
	GRect r = {
		.origin.x = xsmcToInteger(xsArg(0)),
		.origin.y = xsmcToInteger(xsArg(1)),
		.size.w = xsmcToInteger(xsArg(2)),
		.size.h = xsmcToInteger(xsArg(3)),
	};
	int color = xsmcToInteger(xsArg(4));
	int radius = (xsmcArgc > 5) ? xsmcToInteger(xsArg(5)) : 4;

	GColor saveColor = ctx->draw_state.stroke_color;
	ctx->draw_state.stroke_color.argb = color;
	graphics_draw_round_rect(ctx, &r, radius);
	ctx->draw_state.stroke_color = saveColor;
}

void xs_pocopebbble_drawCircle(xsMachine *the)
{
	GContext *ctx = getPocoGContext(the);
	int color = xsmcToInteger(xsArg(0));
	GPoint center = {
		.x = xsmcToInteger(xsArg(1)),
		.y = xsmcToInteger(xsArg(2)),
	};
	int radius = xsmcToInteger(xsArg(3));
	int from = (xsmcArgc > 4) ? DEG_TO_TRIGANGLE(xsmcToInteger(xsArg(4))) : 0;
	int to = (xsmcArgc > 5) ? DEG_TO_TRIGANGLE(xsmcToInteger(xsArg(5))) : TRIG_MAX_ANGLE;

	GColor saveColor = ctx->draw_state.fill_color;
	ctx->draw_state.fill_color.argb = color;
	if ((0 == from) && (TRIG_MAX_ANGLE == to))
		graphics_fill_circle(ctx, center, radius);
	else {
		GRect r = {
			.origin.x = center.x - radius,
			.origin.y = center.y - radius,
			.size.w = radius << 1,
			.size.h = radius << 1,
		};
		const int32_t inset_thickness = MAX(ABS(r.size.h), ABS(r.size.w));
		graphics_fill_radial(ctx, r, GOvalScaleModeFillCircle, inset_thickness, from, to);
	}
	ctx->draw_state.fill_color = saveColor;
}

void xs_pocopebbble_drawDCI(xsMachine *the)
{
	GContext *ctx = getPocoGContext(the);
	GPoint offset = {
		.x = xsmcToInteger(xsArg(1)),
		.y = xsmcToInteger(xsArg(2)),
	};
	xsDestructor d = xsGetHostDestructor(xsArg(0));
	if (d == xs_pebbledci_destructor) {
		GDrawCommandImage *dci = xsmcGetHostDataValidate(xsArg(0), xs_pebbledci_destructor);
		gdraw_command_image_draw(ctx, dci, offset);
	}
	else if (d == xs_pebbledcs_destructor) {
		GDrawCommandSequence *dcs = xsmcGetHostDataValidate(xsArg(0), xs_pebbledcs_destructor);
		xsmcGet(xsResult, xsArg(0), xsID_time);
		xsIntegerValue time = xsmcToInteger(xsResult);
		GDrawCommandFrame *frame = gdraw_command_sequence_get_frame_by_elapsed(dcs, time);
		gdraw_command_frame_draw(ctx, dcs, frame, offset);
	}
	else {
		GDrawCommandList *list = xsmcGetHostDataValidate(xsArg(0), xs_pebbledcl_destructor);
		graphics_context_move_draw_box(ctx, offset);
		gdraw_command_list_draw(ctx, list);
		graphics_context_move_draw_box(ctx, GPoint(-offset.x, -offset.y));
	}
}

void xs_pebblebitmap_build(xsMachine *the)
{
	int id = xsmcToInteger(xsArg(1));
	GBitmap *bitmap = gbitmap_create_with_resource(id);
	if (!bitmap)
		xsUnknownError("not found");

	CommodettoBitmap cb = xsmcGetHostChunk(xsArg(0));

	cb->w = bitmap->bounds.size.w;
	cb->h = bitmap->bounds.size.h;
	cb->bits.data = bitmap;
	cb->format = kCommodettoBitmapPebble;
	cb->havePointer = true;
	cb->bufferSlot = C_NULL;
	cb->byteLength = 0;

	xsResult = xsArg(0);
}

void xs_pebbledci_destructor(void *data)
{
	if (data)
		gdraw_command_image_destroy((GDrawCommandImage *)data);
}

void xs_pebbledci(xsMachine *the)
{
	int id = xsmcToInteger(xsArg(0));
	GDrawCommandImage *dc = gdraw_command_image_create_with_resource(id);
	if (!dc)
		xsUnknownError("not found");
	
	xsmcSetHostData(xsThis, dc);
}

void xs_pebbledci_get_width(xsMachine *the)
{
	GDrawCommandImage *dc = xsmcGetHostDataValidate(xsThis, xs_pebbledci_destructor);
	GSize size = gdraw_command_image_get_bounds_size(dc);
	xsmcSetInteger(xsResult, size.w);
}

void xs_pebbledci_get_height(xsMachine *the)
{
	GDrawCommandImage *dc = xsmcGetHostDataValidate(xsThis, xs_pebbledci_destructor);
	GSize size = gdraw_command_image_get_bounds_size(dc);
	xsmcSetInteger(xsResult, size.h);
}

void xs_pebbledcs_destructor(void *data)
{
	if (data)
		gdraw_command_sequence_destroy((GDrawCommandSequence *)data);
}

void xs_pebbledcs(xsMachine *the)
{
	int id = xsmcToInteger(xsArg(0));
	GDrawCommandSequence *cs = gdraw_command_sequence_create_with_resource(id);
	if (!cs)
		xsUnknownError("not found");

	xsSlot tmp;
	xsmcSetInteger(tmp, 0);
	xsmcSet(xsThis, xsID_time, tmp);

	xsmcSetHostData(xsThis, cs);
}

void xs_pebbledcs_get_width(xsMachine *the)
{
	GDrawCommandSequence *cs = xsmcGetHostDataValidate(xsThis, xs_pebbledcs_destructor);
	GSize size = gdraw_command_sequence_get_bounds_size(cs);
	xsmcSetInteger(xsResult, size.w);
}

void xs_pebbledcs_get_height(xsMachine *the)
{
	GDrawCommandSequence *cs = xsmcGetHostDataValidate(xsThis, xs_pebbledcs_destructor);
	GSize size = gdraw_command_sequence_get_bounds_size(cs);
	xsmcSetInteger(xsResult, size.h);
}

void xs_pebbledcs_get_duration(xsMachine *the)
{
	GDrawCommandSequence *cs = xsmcGetHostDataValidate(xsThis, xs_pebbledcs_destructor);
	uint32_t duration = gdraw_command_sequence_get_total_duration(cs);
	if (PLAY_DURATION_INFINITE == duration)
		xsmcSetNumber(xsResult, C_INFINITY);
	else
		xsmcSetInteger(xsResult, duration);
}

void xs_pebbledcs_get_frameDuration(xsMachine *the)
{
	GDrawCommandSequence *cs = xsmcGetHostDataValidate(xsThis, xs_pebbledcs_destructor);
	xsmcGet(xsResult, xsThis, xsID_time);
	xsIntegerValue time = xsmcToInteger(xsResult);
	GDrawCommandFrame *frame = gdraw_command_sequence_get_frame_by_elapsed(cs, time);
	xsmcSetInteger(xsResult, (xsIntegerValue)gdraw_command_frame_get_duration(frame));
}

void xs_pebbledci_clone(xsMachine *the)
{
	GDrawCommandList *list;
	xsDestructor d = xsGetHostDestructor(xsThis);
	if (d == xs_pebbledci_destructor) {
		GDrawCommandImage *dci = xsmcGetHostDataValidate(xsThis, xs_pebbledci_destructor);
		list = gdraw_command_image_get_command_list(dci);
	}
	else {
		GDrawCommandSequence *cs = xsmcGetHostDataValidate(xsThis, xs_pebbledcs_destructor);
		xsmcGet(xsResult, xsThis, xsID_time);
		xsIntegerValue time = xsmcToInteger(xsResult);
		GDrawCommandFrame *frame = gdraw_command_sequence_get_frame_by_elapsed(cs, time);
		list = gdraw_command_frame_get_command_list(frame);
	}

	list = gdraw_command_list_clone(list);
	xsResult = xsNewHostInstance(xsArg(0));
	xsmcSetHostData(xsResult, list);
}

static xsIntegerValue xs_argToFixed(xsMachine* the, xsIntegerValue c, xsIntegerValue i, xsStringValue name)
{
	if (c <= i) xsTypeError("missing %s", name);
	xsType t = xsmcTypeOf(xsArg(i)); 
	if (xsIntegerType == t) {
		return xsmcToInteger(xsArg(i)) << 8;
	}
	if (xsNumberType == t) {
		xsNumberValue value = xsmcToNumber(xsArg(i));
		if (c_isfinite(value))
			return value * 256;
	}

	xsTypeError("invalid %s", name);
}

void xs_pebbledcl_destructor(void *data)
{
	gdraw_command_list_destroy((GDrawCommandList *)data);
}

void xs_pebbledcl_scale(xsMachine *the)
{
	GDrawCommandList *list = xsmcGetHostDataValidate(xsThis, xs_pebbledcl_destructor);
	xsIntegerValue c = xsmcArgc;
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = (c < 2) ? x : xs_argToFixed(the, c, 1, "y");
	GSize from = {.w = 256, .h = 256};
	GSize to = {.w = x, .h = y};
	
	gdraw_command_list_scale(list, from, to);

	xsResult = xsThis;
}

struct DCLRotateRecord {
	xsIntegerValue angleSin;
	xsIntegerValue angleCos;
	xsIntegerValue cx;
	xsIntegerValue cy; 
};
typedef struct DCLRotateRecord DCLRotateRecord;
typedef struct DCLRotateRecord *DCLRotate;

static bool doRotate(GDrawCommand *command, uint32_t index, void *context)
{
	DCLRotate rr = context;
	const uint16_t numPoints = gdraw_command_get_num_points(command);
	for (uint16_t i = 0; i < numPoints; i++) {
		GPoint p = gdraw_command_get_point(command, i);
		xsIntegerValue x = p.x - rr->cx, y = p.y - rr->cy;
		p.x = (((x * rr->angleCos) + (y * rr->angleSin)) >> 8) + rr->cx;
		p.y = (((y * rr->angleCos) - (x * rr->angleSin)) >> 8) + rr->cy;
		gdraw_command_set_point(command, i, p);
	}

	return true;
}

void xs_pebbledcl_rotate(xsMachine *the)
{
	GDrawCommandList *list = xsmcGetHostDataValidate(xsThis, xs_pebbledcl_destructor);
	xsNumberValue angle = xsmcToNumber(xsArg(0));
	DCLRotateRecord rr = {
		.angleSin = c_sin(angle) * 256,
		.angleCos = c_cos(angle) * 256,
	};

	if (xsmcArgc >= 3) {
		rr.cx = xsmcToInteger(xsArg(1)) << 3;		// 13.3 fixed!
		rr.cy = xsmcToInteger(xsArg(2)) << 3;
	}

	gdraw_command_list_iterate(list, doRotate, &rr);

	xsResult = xsThis;
}
