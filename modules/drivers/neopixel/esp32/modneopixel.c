/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "neopixel.h"

#ifndef MODDEF_NEOPIXEL_RMT_CHANNEL
	#define MODDEF_NEOPIXEL_RMT_CHANNEL (RMT_CHANNEL_0)
#endif

typedef struct {
	pixel_settings_t	px;

	uint8_t				redShift;
	uint8_t				greenShift;
	uint8_t				blueShift;
	uint8_t				whiteShift;

	uint32_t			pixels[1];
} xsNeoPixelRecord, *xsNeoPixel;

static void setPixel(xsNeoPixel np, uint16_t index, uint32_t color);

#define xsmcGetHostDataNeoPixel(slot) ((void *)((char *)xsmcGetHostData(slot) - offsetof(xsNeoPixelRecord, pixels)))

void xs_neopixel_destructor(void *data)
{
	if (data) {
		xsNeoPixel np = data;

		np_clear(&np->px);
		np_show(&np->px);
		neopixel_deinit(&np->px);
	}
}

void xs_neopixel(xsMachine *the)
{
	xsNeoPixel np;
	int pin = -1, length = 0, wstype;
	pixel_settings_t *px;
	char *order = NULL;
	uint8_t shift;

	xsmcVars(3);
	if (xsmcHas(xsArg(0), xsID_length)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_length);
		length = xsmcToInteger(xsVar(0));
	}
#ifdef MODDEF_NEOPIXEL_LENGTH
	else
		length = MODDEF_NEOPIXEL_LENGTH;
#endif
	if (!length)
		xsRangeError("no pixels");

	if (xsmcHas(xsArg(0), xsID_pin)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pin);
		pin = xsmcToInteger(xsVar(0));
	}
#ifdef MODDEF_NEOPIXEL_PIN
	else
		pin = MODDEF_NEOPIXEL_PIN;
#endif
	if (pin < 0)
		xsRangeError("no pin");

	if (xsmcHas(xsArg(0), xsID_order)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_order);
		if (xsmcTest(xsVar(0)))
			order = xsmcToString(xsVar(0));
	}
#ifdef MODDEF_NEOPIXEL_ORDER
	else
		order = MODDEF_NEOPIXEL_ORDER;
#endif
	if (!order)
		order = "GRB";

	wstype = c_strlen(order);
	if (3 == wstype)
		wstype = 0;
	else if (4 == wstype)
		wstype = 1;
	else
		xsUnknownError("invalid order");

	np = c_calloc(sizeof(xsNeoPixelRecord) + ((length - 1) * sizeof(uint32_t)), 1);
	if (!np)
		xsUnknownError("no memory");
	xsmcSetHostBuffer(xsThis, &np->pixels, length * sizeof(uint32_t));

	px = &np->px;
	px->pixels = (void *)np->pixels;
	px->pixel_count = length;
	px->brightness = 0x40;
	c_strcpy(px->color_order, order);

	shift = wstype ? 24 : 16;
	while ((shift >= 0) && *order) {
		switch(*order++) {
			case 'R':
				np->redShift = shift;
				break;
			case 'G':
				np->greenShift = shift;
				break;
			case 'B':
				np->blueShift = shift;
				break;
			case 'W':
				np->whiteShift = shift;
				break;
		}
		shift -= 8;
	}

	px->nbits = (1 == wstype) ? 32 : 24;
	if (xsmcHas(xsArg(0), xsID_timing)) {
		uint8_t i;

		xsmcGet(xsVar(0), xsArg(0), xsID_timing);
		for (i = 0; i < 3; i++) {
			bit_timing_t *bt;

			if (0 == i) {
				xsmcGet(xsVar(1), xsVar(0), xsID_mark);
				bt = &px->timings.mark;
			}
			else if (1 == i) {
				xsmcGet(xsVar(1), xsVar(0), xsID_reset);
				bt = &px->timings.reset;
			}
			else if (2 == i) {
				xsmcGet(xsVar(1), xsVar(0), xsID_space);
				bt = &px->timings.space;
			}

			xsmcGet(xsVar(2), xsVar(1), xsID_level0);
			bt->level0 = xsmcToInteger(xsVar(2));
			xsmcGet(xsVar(2), xsVar(1), xsID_level1);
			bt->level1 = xsmcToInteger(xsVar(2));
			xsmcGet(xsVar(2), xsVar(1), xsID_duration0);
			bt->duration0 = xsmcToInteger(xsVar(2)) / RMT_PERIOD_NS;
			xsmcGet(xsVar(2), xsVar(1), xsID_duration1);
			bt->duration1 = xsmcToInteger(xsVar(2)) / RMT_PERIOD_NS;
		}
	}
	else {
		px->timings.mark.level0 = 1;
		px->timings.space.level0 = 1;
		px->timings.mark.duration0 = 12;

		if (1 == wstype) {
			px->timings.mark.duration1 = 12;
			px->timings.space.duration0 = 6;
			px->timings.space.duration1 = 18;
			px->timings.reset.duration0 = 900;
			px->timings.reset.duration1 = 900;
		}
		else {
			px->timings.mark.duration1 = 14;
			px->timings.space.duration0 = 7;
			px->timings.space.duration1 = 16;
			px->timings.reset.duration0 = 600;
			px->timings.reset.duration1 = 600;
		}
	}

	px->pin = pin;
	px->rmtChannel = MODDEF_NEOPIXEL_RMT_CHANNEL;
	neopixel_init(px);

	np_show(px);
}

void xs_neopixel_close(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	if (!np) return;
	xs_neopixel_destructor(np);
	xsmcSetHostData(xsThis, NULL);
}

void xs_neopixel_getPixel(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int index = xsmcToInteger(xsArg(0));

	if ((index >= np->px.pixel_count) || (index < 0))
		return;

	if (24 == np->px.nbits) {
		uint8_t *p = (index * 3) + (uint8_t *)np->pixels;
		int color = (p[0] << 16) | (p[1] << 8) | p[2];
		xsmcSetInteger(xsResult, color);
	}
	else
		xsmcSetInteger(xsResult, np->pixels[index]);
}

void xs_neopixel_setPixel(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int index = xsmcToInteger(xsArg(0));
	uint32_t color = xsmcToInteger(xsArg(1));

	if ((index >= np->px.pixel_count) || (index < 0))
		return;

	setPixel(np, (uint16_t)index, color);
}

void xs_neopixel_fill(xsMachine *the)
{
	int argc = xsmcArgc;
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	uint32_t color = xsmcToInteger(xsArg(0));
	int index = 0, count = np->px.pixel_count;

	if (argc > 1) {
		index = xsmcToInteger(xsArg(1));
		if ((index < 0) || (index >= count))
			return;

		if (argc > 2) {
			count = xsmcToInteger(xsArg(2));
			if (count <= 0)
				return;
		}

		if ((index + count) > np->px.pixel_count)
			count = np->px.pixel_count - index;
	}

	while (count--)
		setPixel(np, (uint16_t)index++, color);
}

void xs_neopixel_update(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	np_show(&np->px);
}

void xs_neopixel_brightness_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->px.brightness);
}

void xs_neopixel_brightness_set(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int value = xsmcToInteger(xsArg(0));
	if (value < 0) value = 0;
	else if (value > 255) value = 255;
	np->px.brightness = value;
}

void xs_neopixel_length_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->px.pixel_count);
}

void xs_neopixel_byteLength_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->px.pixel_count * (np->px.nbits >> 3));
}

void xs_neopixel_makeRGB(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int r = (xsmcToInteger(xsArg(0)) & 0xFF) << np->redShift;
	int g = (xsmcToInteger(xsArg(1)) & 0xFF) << np->greenShift;
	int b = (xsmcToInteger(xsArg(2)) & 0xFF) << np->blueShift;

	if (24 == np->px.nbits)
		xsmcSetInteger(xsResult, r | g | b);
	else {
		int w = 0;
		if (xsmcArgc > 3)
			w = xsmcToInteger(xsArg(3)) << np->whiteShift;
		xsmcSetInteger(xsResult, r | g | b | w);
	}
}

void xs_neopixel_makeHSB(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int h = xsmcToInteger(xsArg(0));
	int s = xsmcToInteger(xsArg(1));
	int b = xsmcToInteger(xsArg(2));
	int w = 0;
	int rgb, r, g;

	if (32 == np->px.nbits)
		w = xsmcToInteger(xsArg(3)) << np->whiteShift;

	rgb = hsb_to_rgb_int(h, s, b);

	r = (rgb >> 16) << np->redShift;
	g = ((rgb >> 8) & 0xff) << np->greenShift;
	b = (rgb & 0xff) << np->blueShift;

	xsmcSetInteger(xsResult, r | g | b | w);
}

void setPixel(xsNeoPixel np, uint16_t index, uint32_t color)
{
	if (24 == np->px.nbits) {
		uint8_t *p = (index * 3) + (uint8_t *)np->pixels;
		p[0] = (uint8_t)(color >> 16);
		p[1] = (uint8_t)(color >>  8);
		p[2] = (uint8_t)color;
	}
	else
		np->pixels[index] = color;
}
