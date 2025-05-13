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
#include "applib/graphics/graphics_line.h"
#include "applib/rockyjs/api/rocky_api_graphics_text.h"

extern const RockyAPISystemFontDefinition s_font_definitions[];
extern GContext *getPocoGContext(xsMachine *the);

void xs_pocopebble_Font_destructor(void *data)
{
}

void xs_pocopebble_Font(xsMachine *the)
{
	const char *fontName = xsmcToString(xsArg(0));
	const RockyAPISystemFontDefinition *font_definition = NULL;
	const RockyAPISystemFontDefinition *walker = s_font_definitions;
	while (walker->js_name) {
		if (c_strcmp(fontName, walker->js_name) == 0) {
			font_definition = walker;
			break;
		}
		walker++;
	}
	if (!font_definition)
		xsUnknownError("invalid font");

	GFont font = fonts_get_system_font(font_definition->res_key);
	if (!font)
		xsUnknownError("invalid font");

	xsmcSetHostData(xsThis, font);

	xsSlot tmp;
	xsmcSetInteger(tmp, fonts_get_font_height(font));
	xsmcDefine(xsThis, xsID_height, tmp, xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, 0);		//@@
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
				GTextOverflowModeWordWrap,
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

	GRect box = GRect(xsmcToInteger(xsArg(3)), xsmcToInteger(xsArg(4)), 10000, 100);

	GColor saveTextColor = ctx->draw_state.text_color; 
	ctx->draw_state.text_color.argb = xsmcToInteger(xsArg(2));
	graphics_draw_text(ctx, xsmcToString(xsArg(0)), font, box,
		GTextOverflowModeWordWrap, GTextAlignmentLeft, C_NULL);
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
