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

#include "ws2812.pio.h"

typedef struct {
	uint8_t		redShift;
	uint8_t		greenShift;
	uint8_t		blueShift;
	uint8_t		whiteShift;

	uint16_t	length;
	uint8_t		brightness;
	uint8_t		nbits;
	uint8_t		pin;

	char		color_order[5];

	PIO			pio;
	int			sm;

	uint32_t	pixels[1];		// these must be at the end of the structure
} xsNeoPixelRecord, *xsNeoPixel;

static void setPixel(xsNeoPixel np, uint16_t index, uint32_t color);

#define xsmcGetHostDataNeoPixel(slot) ((void *)((char *)xsmcGetHostData(slot) - offsetof(xsNeoPixelRecord, pixels)))
uint32_t hsb_to_rgb_int(int hue, int sat, int brightness);
static int gNeoPixelsInited = 0;


void xs_neopixel_destructor(void *data)
{
	if (data) {
		xsNeoPixel np = data;

//		np_clear(&np->px);
//		np_show(&np->px);
		// neopixel_deinit(&np->px);
	}
}

void np_init(xsNeoPixel np)
{
	if (1 == gNeoPixelsInited)
		return;

	uint offset = pio_add_program(np->pio, &ws2812_program);

	ws2812_program_init(np->pio, np->sm, offset, np->pin, 800000, np->nbits == 32 ? 1 : 0);
	gNeoPixelsInited = 1;
}

uint32_t np_bright(xsNeoPixel np, uint32_t px)
{
	uint32_t rb = px & 0x00FF00FF;
	uint32_t gw = (px >> 8) & 0x00FF00FF;
	rb *= np->brightness;
	gw *= np->brightness;
	return ((rb >> 8) & 0x00FF00FF) | (gw & 0xFF00FF00);
}

void np_show(xsNeoPixel np)
{
	int i, shift;
	shift = np->nbits == 32 ? 0 : 8;
	for (i=0; i<np->length; i++)
		pio_sm_put_blocking(np->pio, np->sm, np_bright(np, np->pixels[i] << shift));

	sleep_ms(10);
}

void xs_neopixel(xsMachine *the)
{
	xsNeoPixel np;
	int pin, length, wstype;
	char *order;
	uint8_t shift;

	xsmcVars(3);
#ifdef MODDEF_NEOPIXEL_LENGTH
	length = MODDEF_NEOPIXEL_LENGTH;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_length);
	length = xsmcToInteger(xsVar(0));
	if (!length)
		xsRangeError("no pixels");
#endif

#ifdef MODDEF_NEOPIXEL_PIN
	pin = MODDEF_NEOPIXEL_PIN;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));
#endif

#ifdef MODDEF_NEOPIXEL_ORDER
	order = MODDEF_NEOPIXEL_ORDER;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_order);
	if (xsmcTest(xsVar(0)))
		order = xsmcToString(xsVar(0));
	else
		order = "GRB";
#endif

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
	xsmcSetHostData(xsThis, &np->pixels);

	np->length = length;
	np->brightness = 0x40;
	c_strncpy(np->color_order, order, 4);

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

	np->nbits = (1 == wstype) ? 32 : 24;
	np->pin = pin;
	np->pio = pio0;		//@@ make configurable
	np->sm = pio_claim_unused_sm(np->pio, false);
	if (np->sm < 0) {
		np->pio = pio1;
		np->sm = pio_claim_unused_sm(np->pio, true);
	}

	np_init(np);

	np_show(np);
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

	if ((index >= np->length) || (index < 0))
		return;

	if (24 == np->nbits) {
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

	if ((index >= np->length) || (index < 0))
		xsRangeError("invalid");

	setPixel(np, (uint16_t)index, color);
}

void xs_neopixel_fill(xsMachine *the)
{
	int argc = xsmcArgc;
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	uint32_t color = xsmcToInteger(xsArg(0));
	int index = 0, count = np->length;

	if (argc > 1) {
		index = xsmcToInteger(xsArg(1));
		if ((index < 0) || (index >= count))
			xsRangeError("invalid");

		if (argc > 2)
			count = xsmcToInteger(xsArg(2));

		if ((index + count) > np->length)
			count = np->length - index;
	}

	while (count--)
		setPixel(np, (uint16_t)index++, color);
}

void xs_neopixel_update(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	np_show(np);
}

void xs_neopixel_brightness_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->brightness);
}

void xs_neopixel_brightness_set(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int value = xsmcToInteger(xsArg(0));
	if (value < 0) value = 0;
	else if (value > 255) value = 255;
	np->brightness = value;
}

void xs_neopixel_length_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->length);
}

void xs_neopixel_byteLength_get(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	xsmcSetInteger(xsResult, np->length * (np->nbits >> 3));
}

void xs_neopixel_makeRGB(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int r = (xsmcToInteger(xsArg(0)) & 0xFF) << np->redShift;
	int g = (xsmcToInteger(xsArg(1)) & 0xFF) << np->greenShift;
	int b = (xsmcToInteger(xsArg(2)) & 0xFF) << np->blueShift;

	if (24 == np->nbits)
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

	if (32 == np->nbits)
		w = xsmcToInteger(xsArg(3)) << np->whiteShift;

	rgb = hsb_to_rgb_int(h, s, b);

	r = (rgb >> 16) << np->redShift;
	g = ((rgb >> 8) & 0xff) << np->greenShift;
	b = (rgb & 0xff) << np->blueShift;

	xsmcSetInteger(xsResult, r | g | b | w);
}

void setPixel(xsNeoPixel np, uint16_t index, uint32_t color)
{
	if (24 == np->nbits) {
		uint8_t *p = (index * 3) + (uint8_t *)np->pixels;
		p[0] = (uint8_t)(color >> 16);
		p[1] = (uint8_t)(color >>  8);
		p[2] = (uint8_t)color;
	}
	else
		np->pixels[index] = color;
}


// Convert HSB color to 24-bit color representation
// _hue: 0 ~ 359
// _sat: 0 ~ 255
// _bri: 0 ~ 255
//=======================================================
uint32_t hsb_to_rgb_int(int hue, int sat, int brightness)
{
	float _hue = (float)hue;
	float _sat = (float)((float)sat / 1000.0);
	float _brightness = (float)((float)brightness / 1000.0);
	float red = 0.0;
	float green = 0.0;
	float blue = 0.0;

	if (_sat == 0.0) {
		red = _brightness;
		green = _brightness;
		blue = _brightness;
	}
	else {
		if (_hue >= 360.0) _hue = fmod(_hue, 360);

		int slice = (int)(_hue / 60.0);
		float hue_frac = (_hue / 60.0) - slice;

		float aa = _brightness * (1.0 - _sat);
		float bb = _brightness * (1.0 - _sat * hue_frac);
		float cc = _brightness * (1.0 - _sat * (1.0 - hue_frac));

		switch(slice) {
			case 0:
				red = _brightness;
				green = cc;
				blue = aa;
				break;
			case 1:
				red = bb;
				green = _brightness;
				blue = aa;
				break;
			case 2:
				red = aa;
				green = _brightness;
				blue = cc;
				break;
			case 3:
				red = aa;
				green = bb;
				blue = _brightness;
				break;
			case 4:
				red = cc;
				green = aa;
				blue = _brightness;
				break;
			case 5:
				red = _brightness;
				green = aa;
				blue = bb;
				break;
			default:
				red = 0.0;
				green = 0.0;
				blue = 0.0;
				break;
		}
	}

	return (uint32_t)((uint8_t)(red * 255.0) << 16) | ((uint8_t)(green * 255.0) << 8) | ((uint8_t)(blue * 255.0));
}

