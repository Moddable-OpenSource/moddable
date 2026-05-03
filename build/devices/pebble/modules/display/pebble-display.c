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

#include "xsmc.h"
#include "xsHost.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "drivers/display/display.h" // FIXME: Need display dimensions
#include "process_state/app_state/app_state.h"

#if kCommodettoBitmapMonochromeAligned != kCommodettoBitmapFormat
	#error unsupported display pixels
#endif

typedef struct {
	PixelsOutDispatch			dispatch;

	GContext						*ctx;
	
	int16_t						width;
	int16_t						height;

} pblDisplayRecord, *pblDisplay;

static void pebbledisplayBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void pebbledisplayEnd(void *refcon);
static void pebbledisplaySend(PocoPixel *pixels, int byteLength, void *refcon);
static void pebbledisplayAdaptInvalid(void *refcon, CommodettoRectangle r);
static void pebbledisplayBeginFrameBuffer(void *refcon, CommodettoPixel **pixels, int16_t *rowBytes);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	pebbledisplayBegin,
	pebbledisplayEnd,
	pebbledisplayEnd,
	pebbledisplaySend,
	pebbledisplayAdaptInvalid,
	pebbledisplayBeginFrameBuffer
};

void xs_pebbledisplay_destructor(void *data)
{
	pblDisplay pd = data;
	if (!pd) return;


	c_free(pd);
}

void xs_pebbledisplay(xsMachine *the)
{
	pblDisplay pd;

	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		if (kCommodettoBitmapMonochrome != xsmcToInteger(xsVar(0)))
			xsUnknownError("bad format");
	}

	pd = c_calloc(1, sizeof(pblDisplayRecord));
	if (!pd)
		xsUnknownError("no memory");

	pd->ctx = app_state_get_graphics_context();
	pd->width = pd->ctx->dest_bitmap.bounds.size.w;
	pd->height = pd->ctx->dest_bitmap.bounds.size.h;

	xsmcSetHostData(xsThis, pd);

	pd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;
}

void xs_pebbledisplay_begin(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	pebbledisplayBegin(pd, x, y, w, h);
}

void xs_pebbledisplay_send(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	const uint8_t *data;
	xsUnsignedValue count;
	xsIntegerValue offset = 0;
	xsIntegerValue c = -1;

	if (argc > 1) {
		offset = xsmcToInteger(xsArg(1));
		if (argc > 2)
			c = xsmcToInteger(xsArg(2));
	}

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &count);

	if (argc > 1) {
		if ((xsUnsignedValue)offset >= count)
			xsUnknownError("bad offset");
		data += offset;
		count -= offset;
		if (argc > 2) {
			if (c > (xsIntegerValue)count)
				xsUnknownError("bad count");
			count = c;
		}
	}

	(pd->dispatch->doSend)((PocoPixel *)data, count, pd);
}

void xs_pebbledisplay_end(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	pebbledisplayEnd(pd);
}

void xs_pebbledisplay_adaptInvalid(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);

	if (pd->dispatch->doAdaptInvalid) {
		CommodettoRectangle invalid = xsmcGetHostChunk(xsArg(0));
		(pd->dispatch->doAdaptInvalid)(pd, invalid);
	}
}

void xs_pebbledisplay_pixelsToBytes(xsMachine *the)
{
	int count = xsmcToInteger(xsArg(0));
	
	xsmcSetInteger(xsResult, sizeof(uint32_t) * ((count + 31) >> 5));
}

void xs_pebbledisplay_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_pebbledisplay_get_width(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, pd->width);
}

void xs_pebbledisplay_get_height(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, pd->height);
}

void xs_pebbledisplay_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void xs_pebbledisplay_close(xsMachine *the)
{
	pblDisplay pd = xsmcGetHostData(xsThis);
	if (!pd) return;
	xs_pebbledisplay_destructor(pd);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pebbledisplay_color_get(xsMachine *the)
{
#if PBL_COLOR
	xsResult = xsTrue;
#elif PBL_BW
	xsResult = xsFalse;
#else
	#error PBL_COLOR or PBL_BW expected
#endif
}

void xs_pebbledisplay_round_get(xsMachine *the)
{
#if PBL_RECT
	xsResult = xsFalse;
#elif PBL_ROUND
	xsResult = xsTrue;
#else
	#error PBL_RECT or PBL_ROUND expected
#endif
}

void pebbledisplaySend(PocoPixel *pixels, int byteLength, void *refcon)
{
	// pblDisplay pd = refcon;
}


void pebbledisplayBeginFrameBuffer(void *refcon, CommodettoPixel **pixels, int16_t *rowBytes)
{
	pblDisplay pd = refcon;

	*pixels = pd->ctx->dest_bitmap.addr;
	*rowBytes = pd->ctx->dest_bitmap.row_size_bytes;		
}

void pebbledisplayBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
}

void pebbledisplayEnd(void *refcon)
{
}

void pebbledisplayAdaptInvalid(void *refcon, CommodettoRectangle invalid)
{
	// pblDisplay pd = refcon;

	// left clipped
	if (invalid->x < 0) {
		if (-invalid->x >= invalid->w) {
			invalid->w = 0;
			return;
		}
		invalid->w += invalid->x;
		invalid->x = 0;
	}
#if 0
@@
	// right clipped
	if ((invalid->x + invalid->w) > MODDEF_pebbledisplay_WIDTH) {
		if (MODDEF_pebbledisplay_WIDTH <= invalid->x) {
			invalid->w = 0;
			return;
		}
		invalid->w = MODDEF_pebbledisplay_WIDTH - invalid->x;
	}
#endif
}

void xs_pebbledisplay_get_unobstructed_x(xsMachine *the)
{
	GRect bounds;
	layer_get_unobstructed_bounds(window_get_root_layer(window_stack_get_top_window(app_state_get_window_stack())), &bounds);
	xsmcSetInteger(xsResult, bounds.origin.x);
}

void xs_pebbledisplay_get_unobstructured_y(xsMachine *the)
{
	GRect bounds;
	layer_get_unobstructed_bounds(window_get_root_layer(window_stack_get_top_window(app_state_get_window_stack())), &bounds);
	xsmcSetInteger(xsResult, bounds.origin.y);
}

void xs_pebbledisplay_get_unobstructed_width(xsMachine *the)
{
	GRect bounds;
	layer_get_unobstructed_bounds(window_get_root_layer(window_stack_get_top_window(app_state_get_window_stack())), &bounds);
	xsmcSetInteger(xsResult, bounds.size.w);
}

void xs_pebbledisplay_get_unobstructured_height(xsMachine *the)
{
	GRect bounds;
	layer_get_unobstructed_bounds(window_get_root_layer(window_stack_get_top_window(app_state_get_window_stack())), &bounds);
	xsmcSetInteger(xsResult, bounds.size.h);
}


