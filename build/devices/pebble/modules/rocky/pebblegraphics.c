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

#include "pebblegraphics.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"

#include "applib/graphics/gcolor_definitions.h"
#include "applib/ui/layer.h"
#include "applib/ui/window.h"
#include "applib/ui/app_window_stack.h"
#include "font_resource_keys.auto.h"
#include "system/logging.h"
#include "process_state/app_state/app_state.h"


/*
	Context
*/

// update area is fb.dirty_rect
//
//  struct {
//    uint8_t b:2; //!< Blue
//    uint8_t g:2; //!< Green
//    uint8_t r:2; //!< Red
//    uint8_t a:2; //!< Alpha. 3 = 100% opaque, 2 = 66% opaque, 1 = 33% opaque, 0 = transparent.
//@@ alpha is high bits - 0b11000000
//  };

void pebble_graphics_context_destructor(void *data)
{	
}

static void pebble_graphics_context_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
	PebbleGraphicsContext pgr = it;
	if (pgr->onUpdate)
		(*markRoot)(the, pgr->onUpdate);
}

static const xsHostHooks xsPebbleGraphicsContextHooks = {
	pebble_graphics_context_destructor,
	pebble_graphics_context_mark,
	NULL
};

//@@ must be a better way to get context here
static void doUpdate(Layer *layer, GContext *ctx)
{
	Window *w = app_window_stack_get_top_window();
	PebbleGraphicsContext pgr = window_get_user_data(w);

	xsBeginHost(pgr->the);
	xsCallFunction0(xsReference(pgr->onUpdate), pgr->obj);
	xsEndHost(pgr->the);
}

void pebble_graphics_context(xsMachine *the)
{
	PebbleGraphicsContext pgr = c_calloc(1, sizeof(PebbleGraphicsContextRecord));
	if (!pgr)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, pgr);

	pgr->the = the;
	pgr->obj = xsThis;

	pgr->ctx = app_state_get_graphics_context();
	pgr->w = app_window_stack_get_top_window();
	window_set_user_data(pgr->w, pgr);

	xsSetHostHooks(xsThis, &xsPebbleGraphicsContextHooks);

	if (xsmcHas(xsArg(0), xsID_onUpdate)) {
		pgr->onUpdate = builtinGetCallback(the, xsID_onUpdate);

		Layer *layer = window_get_root_layer(pgr->w);
		layer_set_update_proc(layer, doUpdate);
	}
}

Fixed_S16_3 prv_fixed_s3_from_double(double d)
{
	return (Fixed_S16_3 ){.raw_value = round(d * FIXED_S16_3_FACTOR)};
}

void prv_graphics_return_color_string(xsMachine *the, GColor8 color)
{
	char str[16];
	if (color.a <= 1)
		strncpy(str, "transparent", sizeof(str));
	else
		snprintf(str, sizeof(str), "#%02X%02X%02X", color.r * 85, color.g * 85, color.b * 85);

	xsmcSetString(xsResult, str);
}

void pebble_graphics_context_get_fillStyle(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	prv_graphics_return_color_string(the, pgr->ctx->draw_state.fill_color);
}

//@@ merge with pebble_graphics_context_set_fillStyle 
void pebble_graphics_context_set_strokeStyle(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	GColor color;

	int type = xsmcTypeOf(xsArg(0));
	if ((type == xsIntegerType) || (type == xsNumberType)) {
		color.argb = xsmcToInteger(xsArg(0));
	}
	else {
		char *c = xsmcToString(xsArg(0));
		if (!rocky_api_graphics_color_parse(c, &color))
			xsUnknownError("invalid");
	}

	graphics_context_set_stroke_color(pgr->ctx, color);
}

void pebble_graphics_context_get_strokeStyle(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	prv_graphics_return_color_string(the, pgr->ctx->draw_state.stroke_color);
}

void pebble_graphics_context_set_fillStyle(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	GColor color;

	int type = xsmcTypeOf(xsArg(0));
	if ((type == xsIntegerType) || (type == xsNumberType)) {
		color.argb = xsmcToInteger(xsArg(0));
	}
	else {
		char *c = xsmcToString(xsArg(0));
		if (!rocky_api_graphics_color_parse(c, &color))
			xsUnknownError("invalid");
	}

	graphics_context_set_fill_color(pgr->ctx, color);
}

void pebble_graphics_context_get_lineWidth(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, pgr->ctx->draw_state.stroke_width);
}

void pebble_graphics_context_set_lineWidth(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	graphics_context_set_stroke_width(pgr->ctx, xsmcToInteger(xsArg(0)));
}

void pebble_graphics_context_save(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	Context2DStoredState *node = c_calloc(1, sizeof(Context2DStoredState));
	if (!node)
		xsUnknownError("no memory");
	node->draw_state = pgr->ctx->draw_state;
	node->text_state = pgr->text_state;		//@@ not done in Pebble implementation
	node->next = pgr->storedStates; 
	pgr->storedStates = node;
}

void pebble_graphics_context_restore(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	Context2DStoredState *node = pgr->storedStates;
	if (!node) return;

	pgr->ctx->draw_state = node->draw_state;
	pgr->text_state = node->text_state;		//@@ not done in Pebble implementation
	pgr->storedStates = node->next;
	c_free(node);
}

void pebble_graphics_context_fillRect(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	Fixed_S16_3 v[4];
	for (int i = 0; i < 4; ++i) {
		const double d = xsmcToNumber(xsArg(i));
//@@		if (!prv_check_value_number_within_bounds(RockyArgTypeFixedS16_3, &d, value_error_out)) {
//@@			value_error_out->arg_offset = i;
//@@			return false;
//@@		}
		v[i] = prv_fixed_s3_from_double(d);
	}

	GRectPrecise gp;
	gp.origin.x = v[0];
	gp.origin.y = v[1];
	gp.size.w = v[2];
	gp.size.h = v[3];

	const int16_t x = Fixed_S16_3_rounded_int(gp.origin.x);
	const int16_t y = Fixed_S16_3_rounded_int(gp.origin.y);
	const int16_t w = Fixed_S16_3_rounded_int(grect_precise_get_max_x(&gp)) - x;
	const int16_t h = Fixed_S16_3_rounded_int(grect_precise_get_max_y(&gp)) - y;
	GRect r = GRect(x, y, w, h);		//@@

	graphics_fill_rect(pgr->ctx, &r);
}

void pebble_graphics_context_clearRect(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	const GColor prev_color = pgr->ctx->draw_state.fill_color;
	pgr->ctx->draw_state.fill_color = GColorRed;		//@@
	pebble_graphics_context_fillRect(the);
	pgr->ctx->draw_state.fill_color = prev_color;
}

void pebble_graphics_context_rockyFillRadial(xsMachine *the)
{
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
  GPointPrecise center;
  center.x = prv_fixed_s3_from_double(xsmcToNumber(xsArg(0)));
  center.y = prv_fixed_s3_from_double(xsmcToNumber(xsArg(1)));
  Fixed_S16_3 radius1 = prv_fixed_s3_from_double(xsmcToNumber(xsArg(2)));
  Fixed_S16_3 radius2 = prv_fixed_s3_from_double(xsmcToNumber(xsArg(3)));
  double angle_1 = adjustAngle(xsmcToNumber(xsArg(4)));
  double angle_2 = adjustAngle(xsmcToNumber(xsArg(5)));

  // adjust for coordinate system
  center.x.raw_value -= FIXED_S16_3_HALF.raw_value;
  center.y.raw_value -= FIXED_S16_3_HALF.raw_value;

  radius1.raw_value = MAX(0, radius1.raw_value);
  radius2.raw_value = MAX(0, radius2.raw_value);
  const Fixed_S16_3 inner_radius = Fixed_S16_3(MIN(radius1.raw_value, radius2.raw_value));
  const Fixed_S16_3 outer_radius = Fixed_S16_3(MAX(radius1.raw_value, radius2.raw_value));

  graphics_fill_radial_precise_internal(pgr->ctx, center, inner_radius, outer_radius,
                                        (int32_t)angle_1, (int32_t)angle_2);
}

/*
	drawImage(image, dx, dy)
	drawImage(image, dx, dy, dWidth, dHeight)
	drawImage(image, sx, sy, sWidth, sHeight, dx, dy, dWidth, dHeight)
*/

void pebble_graphics_context_drawImage(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
//@@	int x = xsmcToInteger(xsArg(1));
//@@	int y = xsmcToInteger(xsArg(2));
	GBitmap *bitmap = xsmcGetHostChunk(xsArg(0));		// can move. be careful.
	GRect dst = gbitmap_get_bounds(bitmap);
	graphics_draw_bitmap_in_rect(pgr->ctx, bitmap, &dst);
}

void pebble_graphics_context_fillText(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	// we don't use INT16_MAX as this seems to leads to overflows deep down in our code
	const int16_t large_int = 10000;
	int16_t x = xsmcToInteger(xsArg(1));
	int16_t y = xsmcToInteger(xsArg(2));
	int16_t box_width = (xsmcArgc > 3) ? xsmcToInteger(xsArg(3)) : large_int;
	char *str_buffer = xsmcToString(xsArg(0));
	GRect box = {
		.origin.x = x,
		.origin.y = y,
		.size.w = box_width,
		.size.h = large_int,
	};

	// adjust box to accommodate for alignment
	switch (pgr->text_state.alignment) {
		case GTextAlignmentCenter: {
			box.origin.x -= box.size.w / 2;
			break;
		}
		case GTextAlignmentRight: {
			box.origin.x -= box.size.w;
			break;
		}
		default: {} // do nothing
	}
	pgr->ctx->draw_state.text_color = pgr->ctx->draw_state.fill_color;
	graphics_draw_text(pgr->ctx, str_buffer, pgr->text_state.font,
		box,
		pgr->text_state.overflow_mode,
		pgr->text_state.alignment,
		pgr->text_state.text_attributes);
}

//@@ this follows the RockyJS API. It is pretty far from the Canvas standard
void pebble_graphics_context_measureText(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	const int16_t box_x = argc >= 2 ? (int16_t)xsmcToInteger(xsArg(1)) : 0;
	const int16_t box_y = argc >= 3 ? (int16_t)xsmcToInteger(xsArg(2)) : 0;
	const int16_t box_width = argc >= 4 ? (int16_t)xsmcToInteger(xsArg(3)) : INT16_MAX;
	const GRect box = {
		.origin.x = box_x,
		.origin.y = box_y,
		.size.w = box_width,
		.size.h = INT16_MAX,
	};
	const GSize size = graphics_text_layout_get_max_used_size(pgr->ctx, xsmcToString(xsArg(0)),
                                                pgr->text_state.font,
                                                box, pgr->text_state.overflow_mode,
                                                pgr->text_state.alignment, NULL);

	xsmcSetNewObject(xsResult);
	xsSlot temp;
	xsmcSetInteger(temp, size.w);
	xsmcSet(xsResult, xsID_width, temp);
	xsmcSetInteger(temp, size.h);
	xsmcSet(xsResult, xsID_height, temp);
}

void pebble_graphics_context_get_font(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	if (pgr->text_state.font_name)
		xsmcSetString(xsResult, (char *)pgr->text_state.font_name);
}

/* T_STATIC */ const RockyAPISystemFontDefinition s_font_definitions[] = {
  {.js_name = "18px bold Gothic", .res_key = FONT_KEY_GOTHIC_18_BOLD},
  {.js_name = "14px Gothic", .res_key = FONT_KEY_GOTHIC_14},
  {.js_name = "14px bold Gothic", .res_key = FONT_KEY_GOTHIC_14_BOLD},
  {.js_name = "18px Gothic", .res_key = FONT_KEY_GOTHIC_18},
  {.js_name = "24px Gothic", .res_key = FONT_KEY_GOTHIC_24},
  {.js_name = "24px bold Gothic", .res_key = FONT_KEY_GOTHIC_24_BOLD},
  {.js_name = "28px Gothic", .res_key = FONT_KEY_GOTHIC_28},
  {.js_name = "28px bold Gothic", .res_key = FONT_KEY_GOTHIC_28_BOLD},
  {.js_name = "30px bolder Bitham", .res_key = FONT_KEY_BITHAM_30_BLACK},
  {.js_name = "42px bold Bitham", .res_key = FONT_KEY_BITHAM_42_BOLD},
  {.js_name = "42px light Bitham", .res_key = FONT_KEY_BITHAM_42_LIGHT},
  {.js_name = "42px Bitham-numeric", .res_key = FONT_KEY_BITHAM_42_MEDIUM_NUMBERS},
  {.js_name = "34px Bitham-numeric", .res_key = FONT_KEY_BITHAM_34_MEDIUM_NUMBERS},
  {.js_name = "21px Roboto", .res_key = FONT_KEY_ROBOTO_CONDENSED_21},
  {.js_name = "49px Roboto-subset", .res_key = FONT_KEY_ROBOTO_BOLD_SUBSET_49},
  {.js_name = "28px bold Droid-serif", .res_key = FONT_KEY_DROID_SERIF_28_BOLD},
  {.js_name = "20px bold Leco-numbers", .res_key = FONT_KEY_LECO_20_BOLD_NUMBERS},
  {.js_name = "26px bold Leco-numbers-am-pm", .res_key = FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM},
  {.js_name = "32px bold numbers Leco-numbers", .res_key = FONT_KEY_LECO_32_BOLD_NUMBERS},
  {.js_name = "36px bold numbers Leco-numbers", .res_key = FONT_KEY_LECO_36_BOLD_NUMBERS},
  {.js_name = "38px bold numbers Leco-numbers", .res_key = FONT_KEY_LECO_38_BOLD_NUMBERS},
  {.js_name = "42px bold numbers Leco-numbers", .res_key = FONT_KEY_LECO_42_NUMBERS},
  {.js_name = "28px light numbers Leco-numbers", .res_key = FONT_KEY_LECO_28_LIGHT_NUMBERS},
  { 0 }, // element to support unit-testing
};

void pebble_graphics_context_set_font(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	const char *font = xsmcToString(xsArg(0));
	const RockyAPISystemFontDefinition *font_definition = NULL;
	const RockyAPISystemFontDefinition *walker = s_font_definitions;
	while (walker->js_name) {
		if (c_strcmp(font, walker->js_name) == 0) {
			font_definition = walker;
			break;
		}
		walker++;
	}
	if (!font_definition)
		return;

	pgr->text_state.font = fonts_get_system_font(font_definition->res_key);
	pgr->text_state.font_name = font_definition->js_name;
}

void pebble_graphics_context_get_textAlign(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	char *align_str = C_NULL;
	
	switch (pgr->text_state.alignment) {
		case GTextAlignmentLeft:
			align_str = "left";
			break;
		case GTextAlignmentRight:
			align_str = "right";
			break;
		case GTextAlignmentCenter:
			align_str = "center";
			break;
	}
	if (align_str)
		xsmcSetStringX(xsResult, align_str);
}

void pebble_graphics_context_set_textAlign(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	const char *alignment = xsmcToString(xsArg(0));
	if (!c_strcmp(alignment, "left"))
		pgr->text_state.alignment = GTextAlignmentLeft;
	else if (!c_strcmp(alignment, "right"))
		pgr->text_state.alignment = GTextAlignmentRight;
	else if (!c_strcmp(alignment, "center"))
		pgr->text_state.alignment = GTextAlignmentCenter;
	else if (!c_strcmp(alignment, "start"))
		pgr->text_state.alignment = GTextAlignmentLeft;
	else if (!c_strcmp(alignment, "end"))
		pgr->text_state.alignment = GTextAlignmentRight;
	else
		xsRangeError("invalid");
}

void pebble_graphics_context_get_dirty(xsMachine *the)
{
	// PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	xsUnknownError("not implemented");
}

void pebble_graphics_context_set_dirty(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	layer_mark_dirty(&pgr->w->layer);
}

void pebble_graphics_canvas_destructor(void *data)
{
}

void pebble_graphics_canvas_get_clientWidth(xsMachine *the)
{
	xsmcSetInteger(xsResult, DISP_COLS);
}

void pebble_graphics_canvas_get_clientHeight(xsMachine *the)
{
	xsmcSetInteger(xsResult, DISP_ROWS);
}

void pebble_graphics_canvas_get_unobstructedWidth(xsMachine *the)
{
	xsmcSetInteger(xsResult, DISP_COLS);
}

void pebble_graphics_canvas_get_unobstructedHeight(xsMachine *the)
{
	xsmcSetInteger(xsResult, DISP_ROWS);
}
