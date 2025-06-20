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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "commodettoBitmap.h"
#include "commodettoPoco.h"
#include "commodettoPocoBlit.h"

#include "applib/graphics/gcontext.h"
#include "applib/graphics/gtypes.h"
#include "applib/graphics/graphics.h"
#include "applib/graphics/graphics_line.h"
#include "util/trig.h"

extern GContext *getPocoGContext(xsMachine *the);

void xs_pocopebble_Font_destructor(void *data)
{
}

void xs_pocopebble_Font(xsMachine *the)
{
	int size = xsmcToInteger(xsArg(1));
	const char *family = xsmcToString(xsArg(0));
	int32_t ascent, descent, leading;
	GFont font = modFindPebbleFont(family, size, &ascent, &descent, &leading);
	if (!font)
		xsUnknownError("invalid font");

	xsmcSetHostData(xsThis, font);

	xsSlot tmp;
	xsmcSetInteger(tmp, ascent + descent + leading);
	xsmcDefine(xsThis, xsID_height, tmp, xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, ascent);
	xsmcDefine(xsThis, xsID_ascent, tmp, xsDontDelete | xsDontSet);
}

void xs_pocopebble_getTextWidth(xsMachine *the)
{
	if (xs_pocopebble_Font_destructor != xsGetHostDestructor(xsArg(1))) {
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
	if (xs_pocopebble_Font_destructor != xsGetHostDestructor(xsArg(1))) {
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
}
