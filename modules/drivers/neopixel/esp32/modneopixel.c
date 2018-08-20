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
		np_show(&np->px, MODDEF_NEOPIXEL_RMT_CHANNEL);
		neopixel_deinit(MODDEF_NEOPIXEL_RMT_CHANNEL);
	}
}

void xs_neopixel(xsMachine *the)
{
	xsNeoPixel np;
	int pin, length, wstype;
	pixel_settings_t *px;
	char *order;
	uint8_t shift;

	xsmcVars(1);
#ifdef MODDEF_NEOPIXEL_LENGTH
	length = MODDEF_NEOPIXEL_LENGTH
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

	if (ESP_OK != neopixel_init(pin, MODDEF_NEOPIXEL_RMT_CHANNEL))
		xsUnknownError("init failed");

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

	px->timings.mark.level0 = 1;
	px->timings.space.level0 = 1;
	px->timings.mark.duration0 = 12;

    if (wstype == 1) {
    	px->nbits = 32;
    	px->timings.mark.duration1 = 12;
    	px->timings.space.duration0 = 6;
    	px->timings.space.duration1 = 18;
    	px->timings.reset.duration0 = 900;
    	px->timings.reset.duration1 = 900;
    }
    else {
    	px->nbits = 24;
    	px->timings.mark.duration1 = 14;
    	px->timings.space.duration0 = 7;
    	px->timings.space.duration1 = 16;
    	px->timings.reset.duration0 = 600;
    	px->timings.reset.duration1 = 600;
    }

	np_show(px, MODDEF_NEOPIXEL_RMT_CHANNEL);	// pixels are all zero
}

void xs_neopixel_close(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	if (!np) return;
	xs_neopixel_destructor(np);
	xsmcSetHostData(xsThis, NULL);
}

void xs_neopixel_setPixel(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	int index = xsmcToInteger(xsArg(0));
	uint32_t color = xsmcToInteger(xsArg(1));

	if ((index >= np->px.pixel_count) || (index < 0))
		xsRangeError("invalid");

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
			xsRangeError("invalid");

		if (argc > 2)
			count = xsmcToInteger(xsArg(2));

		if ((index + count) > np->px.pixel_count)
			count = np->px.pixel_count - index;
	}

	while (count--)
		setPixel(np, (uint16_t)index++, color);
}

void xs_neopixel_update(xsMachine *the)
{
	xsNeoPixel np = xsmcGetHostDataNeoPixel(xsThis);
	np_show(&np->px, MODDEF_NEOPIXEL_RMT_CHANNEL);
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
	int r = xsmcToInteger(xsArg(0)) << np->redShift;
	int g = xsmcToInteger(xsArg(1)) << np->greenShift;
	int b = xsmcToInteger(xsArg(2)) << np->blueShift;

	if (24 == np->px.nbits)
		xsmcSetInteger(xsResult, r | g | b);
	else {
		int w = xsmcToInteger(xsArg(3)) << np->whiteShift;
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
