/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
#include "commodettoPocoBlit.h"
#include "commodettoPoco.h"

#include "process_state/app_state/app_state.h"
#include "system/logging.h"
#include "system/passert.h"

#if !kPocoFrameBuffer
	#error Poco frame buffer required on pebble
#endif

#define MODDEF_POCO_PEBBLE_CLIPSTACK (8)
#define MODDEF_POCO_PEBBLE_ORIGINSTACK (8)

typedef struct {
	PocoCoordinate	x;
	PocoCoordinate	y;
} PocoPointRecord, *PocoPoint;

struct PocoPebbleRecord {
	GContext 				*ctx;
	Window					*window;

	uint8_t					clipDepth;
	uint8_t					originDepth;

	PocoRectangleRecord	clips[MODDEF_POCO_PEBBLE_CLIPSTACK];
	PocoPointRecord		origins[MODDEF_POCO_PEBBLE_ORIGINSTACK];
};

typedef struct PocoPebbleRecord PocoPebbleRecord;
typedef struct PocoPebbleRecord *PocoPebble;

static void updateStub(Layer *layer, GContext *ctx)
{
}

static PocoPebble getPocoPebble(Poco poco)
{
	if (poco->pebbleState)
		return (PocoPebble)(poco->pebbleState);

	PocoPebble pp = c_calloc(1, sizeof(PocoPebbleRecord));
	pp->ctx = app_state_get_graphics_context();

	pp->window = window_stack_get_top_window(app_state_get_window_stack());		//@@ incorrect for offscreen buffers

	Layer *layer = window_get_root_layer(pp->window);
	layer_set_update_proc(layer, updateStub);

	poco->pebbleState = (void *)pp;
	return pp;
}

GContext *getPocoPebbleGContext(Poco poco)
{
	return getPocoPebble(poco)->ctx;
}

GContext *getPocoGContext(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	return getPocoPebble(poco)->ctx;
}

/*
	here begin the functions to draw
*/

void PocoRectangleFill(Poco poco, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	ctx->draw_state.fill_color.argb = color;
	GRect r = GRect(x, y, w, h);
	graphics_fill_rect(ctx, &r);
}

void PocoPixelDraw(Poco poco, PocoColor color, PocoCoordinate x, PocoCoordinate y)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	ctx->draw_state.fill_color.argb = color;
	graphics_draw_pixel(ctx, GPoint(x, y));
}

static GColor gBitmapGray4Palette[4] = {
	{ argb:0xC0 }, { argb:0x80 }, { argb:0x40 }, { argb:0x00 },
};

void PocoBitmapDraw(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	GCompOp saveMode = ctx->draw_state.compositing_mode;
	GRect saveClip = ctx->draw_state.clip_box;
	GBitmap src;
	GBitmap *gb;
	
	if (kCommodettoBitmapPebble == bits->format) {
		gb = (GBitmap *)bits->pixels;
		ctx->draw_state.compositing_mode = GCompOpSet;		// OpSet allows alpha transparency to work. unsure it is always correct. we'll see.... from a Poco perspecive... it should be DrawMasked that handles alpha
	}
	else if (kCommodettoBitmapMonochromeAligned == bits->format) {
		src.addr = bits->pixels;
		src.row_size_bytes = ((bits->width + 31) >> 5) * 4;
		src.info.format = GBitmapFormat1Bit;
		src.info.version = GBITMAP_VERSION_1;
		src.bounds = GRect(0, 0, bits->width, bits->height);
		gb = &src;
		ctx->draw_state.compositing_mode = GCompOpAssign;
	}
	else if (kCommodettoBitmapGray4 == bits->format) {
		src.addr = bits->pixels;
		src.row_size_bytes = (bits->width + 3) >> 2;
		src.info.format = GBitmapFormat2BitPalette;
		src.info.version = GBITMAP_VERSION_1;
		src.palette = gBitmapGray4Palette;
		src.bounds = GRect(0, 0, bits->width, bits->height);
		gb = &src;
		ctx->draw_state.compositing_mode = GCompOpSet;
	}
	else
		PBL_ASSERT(false, "monochromealigned, gray4, or PebbleBitmap required");

	grect_clip(&ctx->draw_state.clip_box, &GRect(x, y, sw, sh));

	graphics_draw_bitmap_in_rect_processed(ctx, gb, &GRect(x - sx, y - sy, sw + sx, sh + sy), C_NULL);

	ctx->draw_state.clip_box = saveClip;
	ctx->draw_state.compositing_mode = saveMode;
}

void PocoMonochromeBitmapDraw(Poco poco, PocoBitmap bits, PocoMonochromeMode mode, PocoColor fgColor, PocoColor bgColor, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	PBL_ASSERT(kCommodettoBitmapMonochromeAligned == bits->format, "monochromealigned required");

	if ((kPocoMonochromeForeAndBackground == mode) && (fgColor == bgColor)) {
		PocoRectangleFill(poco, fgColor, 255, x, y, sw, sh);
		return;
	}

	GBitmap src = {
		.addr = bits->pixels,
		.row_size_bytes = ((bits->width + 31) >> 5) * 4,
		.info.format = GBitmapFormat1Bit,
		.info.version = GBITMAP_VERSION_1,
		.bounds = GRect(sx, sy, sw, sh)
	};

	GCompOp saveMode = ctx->draw_state.compositing_mode;
	#if PBL_COLOR
		GColor saveColor = ctx->draw_state.tint_color;
	#endif

	if (mode == kPocoMonochromeForeground) {
#if PBL_BW
		ctx->draw_state.compositing_mode = (fgColor == GColorBlack.argb) ?  GCompOpAnd : GCompOpSet;
#elif PBL_COLOR
		ctx->draw_state.tint_color.argb = fgColor;
		ctx->draw_state.compositing_mode = GCompOpTint;
#else
		#error PBL_COLOR or PBL_BW expected
#endif
	}
	else if (mode == kPocoMonochromeBackground)
		ctx->draw_state.compositing_mode = (bgColor == GColorWhite.argb) ? GCompOpOr : GCompOpClear;
	else	// kPocoMonochromeForeAndBackground
		ctx->draw_state.compositing_mode = (fgColor == GColorBlack.argb) ? GCompOpAssign : GCompOpAssignInverted;

	graphics_draw_bitmap_in_rect_processed(ctx, &src, &GRect(x, y, sw, sh), C_NULL);

	ctx->draw_state.compositing_mode = saveMode;
#if PBL_COLOR
	ctx->draw_state.tint_color = saveColor;
#endif
}

void PocoGrayBitmapDraw(Poco poco, PocoBitmap bits, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	if (kCommodettoBitmapMonochromeAligned == bits->format) {		// special case for text
		PocoMonochromeBitmapDraw(poco, bits, kPocoMonochromeForeground, color, color, x, y, sx, sy, sw, sh);
		return;
	}
	if (kCommodettoBitmapGray4 == bits->format) {
		PocoPebble pp = getPocoPebble(poco);
		GContext *ctx = pp->ctx;
		GCompOp saveMode = ctx->draw_state.compositing_mode;
		GRect saveClip = ctx->draw_state.clip_box;
		GRect dstRect = GRect(x, y, sw, sh);
#if PBL_COLOR
		GColor saveColor = ctx->draw_state.tint_color;
#endif
		GBitmap src = {
			.addr = bits->pixels,
			.row_size_bytes = (bits->width + 3) >> 2,
			.info.format = GBitmapFormat2BitPalette,
			.info.version = GBITMAP_VERSION_1,
			.palette = gBitmapGray4Palette,
			.bounds = GRect(sx, sy, sw, sh),
		};

		grect_clip(&ctx->draw_state.clip_box, &dstRect);

#if PBL_BW
		ctx->draw_state.compositing_mode = (color == GColorBlack.argb) ? GCompOpAnd : GCompOpSet;
#elif PBL_COLOR
		ctx->draw_state.tint_color.argb = color;
		ctx->draw_state.compositing_mode = GCompOpTint;
#endif

		graphics_draw_bitmap_in_rect_processed(ctx, &src, &dstRect, C_NULL);

		ctx->draw_state.clip_box = saveClip;
		ctx->draw_state.compositing_mode = saveMode;
#if PBL_COLOR
		ctx->draw_state.tint_color = saveColor;
#endif
		return;
	}
	
	PBL_CROAK("unexpected PocoGrayBitmapDraw");
}
// bits is kCommodettoBitmapARGB2222 (GBitmapFormat8Bit)
// mask is kCommodettoBitmapGray4 (GBitmapFormat2BitPalette with gBitmapGray4Palette)

void PocoBitmapDrawMasked(Poco poco, uint8_t blend, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh,
			PocoBitmap mask, PocoDimension mask_sx, PocoDimension mask_sy)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	GBitmap gb;
	PBL_ASSERT(kCommodettoBitmapMonochromeAligned == bits->format, "monochromealigned bits required");
	PBL_ASSERT(kCommodettoBitmapMonochromeAligned == mask->format, "monochromealigned mask required");

	GCompOp saveMode = ctx->draw_state.compositing_mode;
	GRect saveClip = ctx->draw_state.clip_box;
	GRect rect = GRect(x, y, sw, sh);
	
	gb.info.format = GBitmapFormat1Bit;
	gb.info.version = GBITMAP_VERSION_1;
	gb.bounds = GRect(sx, sy, sw, sh);
	
	gb.addr = mask->pixels;
	gb.row_size_bytes = ((mask->width + 31) >> 5) * 4;
	ctx->draw_state.compositing_mode = GCompOpSet;
	graphics_draw_bitmap_in_rect_processed(ctx, &gb, &rect, C_NULL);
	
	gb.addr = bits->pixels;
	gb.row_size_bytes = ((bits->width + 31) >> 5) * 4;
	ctx->draw_state.compositing_mode = GCompOpAnd;
	graphics_draw_bitmap_in_rect_processed(ctx, &gb, &rect, C_NULL);
	
	ctx->draw_state.clip_box = saveClip;
	ctx->draw_state.compositing_mode = saveMode;
}

void PocoBitmapPattern(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	GCompOp saveMode = ctx->draw_state.compositing_mode;
	GRect saveClip = ctx->draw_state.clip_box;
	GBitmap src;
	GBitmap *gb;

	if (kCommodettoBitmapPebble == bits->format) {
		gb = (GBitmap *)bits->pixels;
		ctx->draw_state.compositing_mode = GCompOpSet;		// OpSet allows alpha transparency to work. unsure it is always correct. we'll see.... from a Poco perspecive... it should be DrawMasked that handles alpha
	}
	else if (kCommodettoBitmapMonochromeAligned == bits->format) {
		src.addr = bits->pixels;
		src.row_size_bytes = ((bits->width + 31) >> 5) * 4;
		src.info.format = GBitmapFormat1Bit;
		src.info.version = GBITMAP_VERSION_1;
		src.bounds = GRect(sx, sy, sw, sh);
		gb = &src;
		ctx->draw_state.compositing_mode = GCompOpAssign;
	}
	else if (kCommodettoBitmapARGB2222 == bits->format) {
		src.addr = bits->pixels;
		src.row_size_bytes = bits->width;
		src.info.format = GBitmapFormat8Bit;
		src.info.version = GBITMAP_VERSION_1;
		src.bounds = GRect(sx, sy, sw, sh);
		gb = &src;
		ctx->draw_state.compositing_mode = GCompOpAssign;
	}
	else if (kCommodettoBitmapGray4 == bits->format) {
		src.addr = bits->pixels;
		src.row_size_bytes = (bits->width + 3) >> 2;
		src.info.format = GBitmapFormat2BitPalette;
		src.info.version = GBITMAP_VERSION_1;
		src.palette = gBitmapGray4Palette;
		src.bounds = GRect(sx, sy, sw, sh);
		gb = &src;
		ctx->draw_state.compositing_mode = GCompOpAssign;
	}
	else
		PBL_ASSERT(false, "monochromealigned or PebbleBitmap required");

	// pattern support in graphics_draw_bitmap_in_rect_processed renders incorrectly, so loop ourselves
	for (int py = y ; py < (y + h); py += sh) {
		for (int px = x ; px < (x + w); px += sw)
			graphics_draw_bitmap_in_rect_processed(ctx, gb, &GRect(px, py, sw, sh), C_NULL);
	}

	ctx->draw_state.clip_box = saveClip;
	ctx->draw_state.compositing_mode = saveMode;
}


void PocoDrawFrame(Poco poco, uint8_t *data, uint32_t dataSize, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PBL_CROAK("unexpected PocoDrawFrame");
}


/*
	BMFont support
*/

int PocoNextFromUTF8(uint8_t **src)
{
	int result;
	uint8_t *s = *src;
	uint8_t c = c_read8(s++);

	if (!(c & 0x80)) {
		*src = s;
		return c;
	}

	if (0xC0 == (c & 0xE0)) { // 2 byte sequence
		result = ((c & 0x1F) << 6) | (c_read8(s) & 0x3F);
		s += 1;
	}
	else if (0xE0 == (c & 0xF0)) { // 3 byte sequence
		result = ((c & 0x0F) << 12) | ((c_read8(s) & 0x3F) << 6) | (c_read8(s + 1) & 0x3F);
		s += 2;
	}
	else if (0xF0 == (c & 0xF1)) { // 4 byte sequence
		result = ((c & 0x07) << 18) | ((c_read8(s) & 0x3F) << 12) | ((c_read8(s + 1) & 0x3F) << 6) | (c_read8(s + 2) & 0x3F);
		s += 3;
	}
	else
		result = 0;

	*src = s;
	return result;
}

const uint8_t *PocoBMFGlyphFromUTF8(uint8_t **src, const uint8_t *chars, int charCount)
{
	int min, max;
	int c = PocoNextFromUTF8(src);
	if (!c)
		return NULL;

	if (c_read8(chars + 19) & 0x40) {
		// one run of continuously numbered characters
		int firstChar = c_read32(chars);
		if (c < firstChar)
			return NULL;

		c -= firstChar;
		if (c >= charCount)
			return NULL;

		return (20 * c) + chars;
	}

	// ascending order, with gaps. binary search.
	min = 0;
	max = charCount;
	do {
		int mid = (min + max) >> 1;
		const uint8_t *cc = (20 * mid) + chars;
		int code = c_read32(cc);
		if (code < c)
			min = mid + 1;
		else if (c < code)
			max = mid - 1;
		else
			return cc;
	} while (min <= max);

	return NULL;
}

void PocoDrawingBegin(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PBL_CROAK("UNEXPECTED PocoDrawingBegin");
}

int PocoDrawingEnd(Poco poco, PocoPixel *pixels, int byteLength, PocoRenderedPixelsReceiver pixelReceiver, void *refCon)
{
	PBL_CROAK("UNEXPECTED PocoDrawingEnd");
	return 0;
}

void PocoDrawingBeginFrameBuffer(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoPixel *frameBuffer, int16_t rowBytes)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	ctx->draw_state.clip_box = GRect(x, y, w, h);

	poco->flags |= kPocoFlagDidBegin;

	poco->xOrigin = poco->yOrigin = 0;
}

int PocoDrawingEndFrameBuffer(Poco poco)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	PBL_ASSERT((0 == pp->clipDepth) && (0 == pp->originDepth), "clip or origin not popped");
	PBL_ASSERT(!(poco->flags & kPocoFlagErrorStackProblem), "clip or origin stack failure");

	ctx->draw_state.clip_box = ctx->dest_bitmap.bounds;

	if (pp->window)
		window_schedule_render(pp->window);

	return 0;
}

int PocoClipPush(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	PocoRectangle clip;

	if ((MODDEF_POCO_PEBBLE_CLIPSTACK - 1) == pp->clipDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return 0;
	}

	clip = &pp->clips[pp->clipDepth++];

	// save current clip
	clip->x = poco->x;
	clip->y = poco->y;
	clip->w = poco->w;
	clip->h = poco->h;

	// intersect new clip with current clip
	grect_clip(&ctx->draw_state.clip_box, &GRect(x, y, w, h));

	// apply new clip
	poco->x = ctx->draw_state.clip_box.origin.x;
	poco->y = ctx->draw_state.clip_box.origin.y;
	poco->w = ctx->draw_state.clip_box.size.w;
	poco->h = ctx->draw_state.clip_box.size.h;
	poco->xMax = poco->x + poco->w;
	poco->yMax = poco->y + poco->h;

	return 1;
}

void PocoClipPop(Poco poco)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	PocoRectangle clip;

	if (0 == pp->clipDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return;
	}

	clip = &pp->clips[--pp->clipDepth];
	ctx->draw_state.clip_box = GRect(clip->x, clip->y, clip->w, clip->h);

	poco->x = clip->x;
	poco->y = clip->y;
	poco->w = clip->w;
	poco->h = clip->h;
	poco->xMax = clip->x + clip->w;
	poco->yMax = clip->y + clip->h;
}

void PocoOriginPush(Poco poco, PocoCoordinate x, PocoCoordinate y)
{
	PocoPebble pp = getPocoPebble(poco);

	if ((MODDEF_POCO_PEBBLE_ORIGINSTACK - 1) == pp->originDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return;
	}

	// save current origin
	pp->origins[pp->originDepth].x = poco->xOrigin;
	pp->origins[pp->originDepth].y = poco->yOrigin;
	pp->originDepth += 1;

	// apply new origin
	poco->xOrigin += x;
	poco->yOrigin += y;
}

void PocoOriginPop(Poco poco)
{
	PocoPebble pp = getPocoPebble(poco);

	if (0 == pp->originDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return;
	}

	pp->originDepth -= 1;
	poco->xOrigin = pp->origins[pp->originDepth].x;
	poco->yOrigin = pp->origins[pp->originDepth].y;
}
