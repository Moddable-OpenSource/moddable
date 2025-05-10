/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

#define MODDEF_POCO_PEBBLE_CLIPSTACK (16)
#define MODDEF_POCO_PEBBLE_ORIGINSTACK (16)

typedef struct {
	PocoCoordinate	x;
	PocoCoordinate	y;
} PocoPointRecord, *PocoPoint;

struct PocoPebbleRecord {
	GContext 				*ctx;

	uint8_t					clipDepth;
	uint8_t					originDepth;

	PocoRectangleRecord	clips[MODDEF_POCO_PEBBLE_CLIPSTACK];
	PocoPointRecord		origins[MODDEF_POCO_PEBBLE_ORIGINSTACK];
};

typedef struct PocoPebbleRecord PocoPebbleRecord;
typedef struct PocoPebbleRecord *PocoPebble;

static PocoPebble getPocoPebble(Poco poco)
{
	if (poco->next)
		return (PocoPebble)(poco->next);

	PocoPebble pp = c_calloc(1, sizeof(PocoPebbleRecord));		//@@ orphaned
	pp->ctx = app_state_get_graphics_context();

	poco->next = (void *)pp;
	return pp;
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
	GRect r = GRect(x + poco->xOrigin, y + poco->yOrigin, w, h);
	graphics_fill_rect(ctx, &r);
}

void PocoPixelDraw(Poco poco, PocoColor color, PocoCoordinate x, PocoCoordinate y)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;
	ctx->draw_state.fill_color.argb = color;
	graphics_draw_pixel(ctx, GPoint(x + poco->xOrigin, y + poco->yOrigin));
}

void PocoBitmapDraw(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PBL_ASSERT(kCommodettoBitmapPebble == bits->format, "pebble bitmap required");

	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	x += poco->xOrigin, y += poco->yOrigin;

	GRect saveClip = ctx->draw_state.clip_box;
	ctx->draw_state.compositing_mode = GCompOpAssign;
	grect_clip(&ctx->draw_state.clip_box, &GRect(x, y, sw, sh));

	graphics_draw_bitmap_in_rect_processed(ctx, (GBitmap *)bits->pixels, &GRect(x - sx, y - sy, sw + sx, sh + sy), C_NULL);

	ctx->draw_state.clip_box = saveClip;
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

	if (mode == kPocoMonochromeForeground)
		ctx->draw_state.compositing_mode = (fgColor == GColorBlack.argb) ?  GCompOpAnd : GCompOpSet;
	else if (mode == kPocoMonochromeBackground)
		ctx->draw_state.compositing_mode = (bgColor == GColorWhite.argb) ? GCompOpOr : GCompOpClear;
	else	// kPocoMonochromeForeAndBackground
		ctx->draw_state.compositing_mode = (fgColor == GColorBlack.argb) ? GCompOpAssign : GCompOpAssignInverted;

	graphics_draw_bitmap_in_rect_processed(ctx, &src, &GRect(x + poco->xOrigin, y + poco->yOrigin, sw, sh), C_NULL);

	ctx->draw_state.compositing_mode = saveMode;
}

void PocoGrayBitmapDraw(Poco poco, PocoBitmap bits, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	if (kCommodettoBitmapMonochromeAligned == bits->format) {		// special case for text
		PocoMonochromeBitmapDraw(poco, bits, kPocoMonochromeForeground, color, color, x, y, sx, sy, sw, sh);
		return;
	}
	
	PBL_CROAK("unexpected bitmap");
}

void PocoBitmapDrawMasked(Poco poco, uint8_t blend, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh,
			PocoBitmap mask, PocoDimension mask_sx, PocoDimension mask_sy)
{
	PBL_CROAK("unexpected PocoBitmapDrawMasked");
}

void PocoBitmapPattern(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	PBL_ASSERT(kCommodettoBitmapMonochromeAligned == bits->format, "monochromealigned required");
	GBitmap src = {
		.addr = bits->pixels,
		.row_size_bytes = ((bits->width + 31) >> 5) * 4,
		.info.format = GBitmapFormat1Bit,
		.info.version = GBITMAP_VERSION_1,
		.bounds = GRect(sx, sy, sw, sh)
	};

	GCompOp saveMode = ctx->draw_state.compositing_mode;
	GRect saveClip = ctx->draw_state.clip_box;

	ctx->draw_state.compositing_mode = GCompOpAssign;
	grect_clip(&ctx->draw_state.clip_box, &GRect(x, y, w, h));

	// pattern support in graphics_draw_bitmap_in_rect_processed renders incorrectly, so loop ourselves
	x += poco->xOrigin, y += poco->yOrigin;
	for (int py = y ; py < (y + h); py += sh) {
		for (int px = x ; px < (x + w); px += sw)
			graphics_draw_bitmap_in_rect_processed(ctx, &src, &GRect(px, py, sw, sh), C_NULL);
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
}

int PocoDrawingEndFrameBuffer(Poco poco)
{
	PocoPebble pp = getPocoPebble(poco);
	GContext *ctx = pp->ctx;

	PBL_ASSERT((0 == pp->clipDepth) && (0 == pp->originDepth), "clip or origin not popped");
	PBL_ASSERT(!(poco->flags & kPocoFlagErrorStackProblem), "clip or origin stack failure");

	ctx->draw_state.clip_box = ctx->dest_bitmap.bounds;

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
