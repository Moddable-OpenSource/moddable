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

/*
    To do:
    
		- handle continue in begin
		- lots of pixel format conversion

*/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.devicetree.h"
#include "mc.xs.h"		// for xsID_ values
#include "builtinCommon.h"

#include "display419.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#if kModZephyrDisplayBusCount

typedef struct {
	xsMachine 	*the;
	xsSlot		obj;

	const struct device *device;

	uint8_t		format;
	uint8_t		didBegin:1;
	uint8_t		blank:1;
	uint8_t		xAlignmentWidth:1;
	uint16_t		brightness;

	uint16_t		width;
	uint16_t		height;
	uint16_t		bitsPerPixel;

	uint16_t		updateX;
	uint16_t		updateY;
	uint16_t		updateWidth;
	uint16_t		updateHeight;
} modDisplayRecord, *modDisplay;

void xs_display_destructor(void *data)
{
	modDisplay display = (modDisplay)data;
	if (!display) return;

	if (!display->blank)
		display_blanking_on(display->device);

	c_free(display);
}

// static void xs_display_mark(xsMachine* /* the */, void * /* it */, xsMarkRoot /* markRoot */)
// {
// }

static int displayBegin(void *hostData, int x, int y, int width, int height, void **frameBuffer, int *rowBytes, int flags);
static int displaySend(void *hostData, void *buffer, uint32_t length);
static int displayEnd(void *hostData);
static void displayAdaptInvalid(void *hostData, CommodettoRectangle r);
static int displayGet(void *hostData, int32_t what, void *result);

static const xsDisplayHostHooksRecord xsDisplayHooks = {
	.hooks = {
		xs_display_destructor,
		C_NULL,	// xs_display_mark,
		"display"
	},
	.doBegin = displayBegin,
	.doSend = displaySend,
	.doEnd = displayEnd,
	.doAdaptInvalid = displayAdaptInvalid,
	.doGet = displayGet,
};

void xs_display_constructor(xsMachine *the)
{
	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_port);
	const struct modZephyrDisplay *port = modZephyrGetDisplay(xsmcToString(xsVar(0)));
	if (!port)
		xsUnknownError("invalid port");

	if (!device_is_ready(port->device))
		xsUnknownError("not ready");

	struct display_capabilities capabilities;
	display_get_capabilities(port->device, &capabilities);

	xsmcVars(1);

	modDisplay display = c_calloc(1, sizeof(modDisplayRecord));
	if (!display)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, display);
	xsSetHostHooks(xsThis, &xsDisplayHooks.hooks);

	display->the = the;
	display->device = port->device;
	display->blank = 1;
	display->xAlignmentWidth = (capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) ? 1 : 0;
	display->brightness = 256;

	display->width = capabilities.x_resolution;
	display->height = capabilities.y_resolution;
	display->bitsPerPixel = DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format);
}

void xs_display_close(xsMachine *the)
{
	modDisplay display = xsmcGetHostData(xsThis);
	if (display && xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks)) {
		xs_display_destructor(display);
		xsmcSetHostData(xsThis, NULL);
		xsSetHostDestructor(xsThis, NULL);
	}
}

void xs_display_configure(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_brightness);
	if (xsUndefinedType != xsmcTypeOf(xsVar(0))) {
		xsNumberValue brightness = xsmcToNumber(xsVar(0));
		if ((brightness < 0) || (brightness > 1))
			xsRangeError("invalid brightness");
		if (0 != display_set_brightness(display->device, brightness * 256))
			xsUnknownError("no driver brightness");
		display->brightness = brightness * 256;
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_rotation);
	if (xsUndefinedType != xsmcTypeOf(xsVar(0))) {
		xsNumberValue rotation = xsmcToNumber(xsVar(0));
		enum display_orientation orientation;
		switch ((int)rotation) {
			case   0: orientation = DISPLAY_ORIENTATION_NORMAL; break;
			case  90: orientation = DISPLAY_ORIENTATION_ROTATED_90; break;
			case 180: orientation = DISPLAY_ORIENTATION_ROTATED_180; break;
			case 270: orientation = DISPLAY_ORIENTATION_ROTATED_270; break;
			default: xsUnknownError("invalid rotation");
		}
		if (0 != display_set_orientation(display->device, orientation))
			xsUnknownError("no driver rotation");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_format);
	if (xsUndefinedType != xsmcTypeOf(xsVar(0))) {
		enum display_pixel_format format;
		switch (xsmcToInteger(xsVar(0))) {
			case kCommodettoBitmapRGB565LE:
			case kCommodettoBitmapRGB565BE: format = PIXEL_FORMAT_RGB_565; break;
			case kCommodettoBitmap24RGB: format = PIXEL_FORMAT_RGB_888; break;
			case kCommodettoBitmapGray256: format = PIXEL_FORMAT_L_8; break;
			default: xsUnknownError("invalid format");
		}

		if (0 != display_set_pixel_format(display->device, format))
			xsUnknownError("driver doesn't support format");

		display->bitsPerPixel = DISPLAY_BITS_PER_PIXEL(format);
	}
}

void xs_display_configuration(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);

	struct display_capabilities capabilities;
	display_get_capabilities(display->device, &capabilities);

	xsmcVars(1);
	xsmcSetNewObject(xsResult);

	xsmcSetNumber(xsVar(0), ((xsNumberValue)display->brightness) / 256.0);
	xsmcSet(xsResult, xsID_brightness, xsVar(0));

	int rotation = 0;
	switch (capabilities.current_orientation) {
		case DISPLAY_ORIENTATION_ROTATED_90: rotation = 90; break;
		case DISPLAY_ORIENTATION_ROTATED_180:rotation = 180; break;
		case DISPLAY_ORIENTATION_ROTATED_270:rotation = 270; break;
	}
	xsmcSetInteger(xsVar(0), rotation);
	xsmcSet(xsResult, xsID_rotation, xsVar(0));

	int format = 0;
	switch (capabilities.current_pixel_format) {
		// Zephyr's PIXEL_FORMAT_RGB_565 doesn't distinguish endianness; report whichever 565 the build is configured for.
		case PIXEL_FORMAT_RGB_565:
#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565BE
			format = kCommodettoBitmapRGB565BE;
#else
			format = kCommodettoBitmapRGB565LE;
#endif
			break;
		case PIXEL_FORMAT_RGB_888: format = kCommodettoBitmap24RGB; break;
		case PIXEL_FORMAT_L_8: format = kCommodettoBitmapGray256; break;
	}
	xsmcSetInteger(xsVar(0), format);
	xsmcSet(xsResult, xsID_format, xsVar(0));
}

void xs_display_begin(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);

	int x, y, width, height;
	if (xsmcArgc) {
		xsSlot tmp;
		xsmcGet(tmp, xsArg(0), xsID_x);
		x = xsmcToInteger(tmp);
		xsmcGet(tmp, xsArg(0), xsID_y);
		y = xsmcToInteger(tmp);
		xsmcGet(tmp, xsArg(0), xsID_width);
		width = xsmcToInteger(tmp);
		xsmcGet(tmp, xsArg(0), xsID_height);
		height = xsmcToInteger(tmp);
	}
	else {
		x = y = 0;
		width = display->width;
		height = display->height;
	}

	if (0 != displayBegin(display, x, y, width, height, C_NULL, C_NULL, 0))		//@@ continue flag
		xsUnknownError("invalid");
// xsTrace("xs_display_begin\n");
}

void xs_display_send(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	void *buffer;
	xsUnsignedValue length;
// xsTrace("xs_display_send\n");
	if (xsmcArgc >= 3) {
		int offset = xsmcToInteger(xsArg(1));
		int requestedLength = xsmcToInteger(xsArg(2));
		xsmcGetBufferReadable(xsArg(0), &buffer, &length);
		if ((offset < 0) || (requestedLength <= 0) || ((offset + requestedLength > length)))
			xsRangeError("invalid");
		buffer = offset + (uint8_t *)buffer;
		length = (xsUnsignedValue)requestedLength;
	}
	else
		xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (0 != displaySend(display, buffer, length))
		xsRangeError("invalid");
}

void xs_display_end(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
// xsTrace("xs_display_end\n");

	if (0 != displayEnd(display))
		xsUnknownError("invalid");
}

void xs_display_adaptInvalid(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	CommodettoRectangle cr = xsmcGetHostChunk(xsArg(0));
	displayAdaptInvalid(display, cr);
}

void xs_display_get_width(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsmcSetInteger(xsResult, display->width);
}

void xs_display_get_height(xsMachine *the)
{
	modDisplay display = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsmcSetInteger(xsResult, display->height);
}

int displayBegin(void *hostData, int x, int y, int width, int height, void **frameBuffer, int *rowBytes, int flags)
{
	modDisplay display = hostData;

	if (display->didBegin)
		return -1;

	if ((x < 0) || (y < 0) || (width < 0) || (height <= 0) || ((x + width) > display->width) || (y + height > display->height))
		return -2;

	if (display->xAlignmentWidth && ((0 != x) || (display->width != width)))
		return -3;

	display->updateX = (uint16_t)x;
	display->updateY = (uint16_t)y;
	display->updateWidth = (uint16_t)width;
	display->updateHeight = (uint16_t)height;

	display->didBegin = 1;

	if (frameBuffer)
		*frameBuffer = display_get_framebuffer(display->device);
	if (rowBytes)
		*rowBytes = *frameBuffer ? ((display->width * display->bitsPerPixel) >> 3) : 0;

	return 0;
}

int displaySend(void *hostData, void *buffer, uint32_t length)
{
	modDisplay display = hostData;

	if (!display->didBegin)
		return -1;

	if (length % ((display->bitsPerPixel * display->updateWidth) >> 3))
		return -2;
	int rows = length / ((display->bitsPerPixel * display->updateWidth) >> 3);
	if (rows > display->updateHeight)
		return -3;

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
	if (IS_ENABLED(CONFIG_ST7789V_RGB565)) {
		// LE framebuffer; ST7789V wants MSB-first on the wire — software byteswap.
		// On a BE build the framebuffer already matches the wire order, so no swap.
		uint8_t *pixels = buffer;
		uint32_t remain = length >> 1;
		while (remain--) {
			uint8_t t = pixels[0];
			pixels[0] = pixels[1];
			pixels[1] = t;
			pixels += 2;
		}
	}
#endif

	struct display_buffer_descriptor desc = {
		.buf_size = length,
		.width = display->updateWidth,
		.height = rows,
		.pitch = display->updateWidth,
		.frame_incomplete = 0 != (display->updateHeight - rows),
	};

	if (display_write(display->device, display->updateX, display->updateY, &desc, buffer) < 0)
		return -4;

	display->updateY += rows;
	display->updateHeight -= rows;

	return 0;
}

int displayEnd(void *hostData)
{
	modDisplay display = hostData;

	if (!display->didBegin)
		return -1;

	display->didBegin = 0;

	if (display->blank) {
		display->blank = 0;
		display_blanking_off(display->device);
	}

	return 0;
}

void displayAdaptInvalid(void *hostData, CommodettoRectangle r)
{
	modDisplay display = hostData;

	if (!display->xAlignmentWidth)
		return;

	r->x = 0;
	r->w = display->width;
}

int displayGet(void *hostData, int32_t what, void *result)
{
	modDisplay display = hostData;

	if (1 == what) {		// hasFrameBuffer
		*(uint8_t *)result = display_get_framebuffer(display->device) ? 1 : 0;
		return 0;
	}

	return -1;
}

#else // !kModZephyrDisplayBusCount

void xs_display_constructor(xsMachine *the)
{
	xsUnknownError("no display");
}

void xs_display_destructor(void *) {}
void xs_display_close(xsMachine *the) {}
void xs_display_configure(xsMachine *the) {}
void xs_display_configuration(xsMachine *the) {}
void xs_display_begin(xsMachine *the) {}
void xs_display_end(xsMachine *the) {}
void xs_display_send(xsMachine *the) {}
void xs_display_adaptInvalid(xsMachine *the) {}
void xs_display_get_width(xsMachine *the) {}
void xs_display_get_height(xsMachine *the) {}

#endif
