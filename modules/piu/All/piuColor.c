/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "piuAll.h"

static void PiuHSL2RGB(const uint8_t *hsl, uint8_t *rgb);
static uint32_t PiuStringHexToNum(const char *string, uint32_t i);

#define PiuColorCount 18
static const char* PiuColorNames[PiuColorCount] ICACHE_RODATA_ATTR = {
	"black",
	"silver",
	"gray",
	"white",
	"maroon",
	"red",
	"purple",
	"fuchsia",
	"green",
	"lime",
	"olive",
	"yellow",
	"navy",
	"blue",
	"teal",
	"aqua",
	"orange",
	"transparent"
};

static const PiuColorRecord ICACHE_FLASH_ATTR PiuColorValues[PiuColorCount] = {
	{ 0x00, 0x00, 0x00, 0xff },
	{ 0xc0, 0xc0, 0xc0, 0xff },
	{ 0x80, 0x80, 0x80, 0xff },
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0x80, 0x00, 0x00, 0xff },
	{ 0xff, 0x00, 0x00, 0xff },
	{ 0x80, 0x00, 0x80, 0xff },
	{ 0xff, 0x00, 0xff, 0xff },
	{ 0x00, 0x80, 0x00, 0xff },
	{ 0x00, 0xff, 0x00, 0xff },
	{ 0x80, 0x80, 0x00, 0xff },
	{ 0xff, 0xff, 0x00, 0xff },
	{ 0x00, 0x00, 0x80, 0xff },
	{ 0x00, 0x00, 0xff, 0xff },
	{ 0x00, 0x80, 0x80, 0xff },
	{ 0x00, 0xff, 0xff, 0xff },
	{ 0xff, 0xa5, 0x00, 0xff },
	{ 0x00, 0x00, 0x00, 0x00 }
};

void PiuColorsBlend(PiuColor colors, double state, PiuColor color)
{
	double index = c_floor(state);
	double plus = state - index;
	PiuColor back = &colors[(uint32_t)index];
	if (plus) {
		// @@
		PiuColor fore = &colors[(uint32_t)index + 1];
		double minus = 1 - plus;
		double r = ((double)back->r * minus) + ((double)fore->r * plus);
		double g = ((double)back->g * minus) + ((double)fore->g * plus);
		double b = ((double)back->b * minus) + ((double)fore->b * plus);
		double a = ((double)back->a * minus) + ((double)fore->a * plus);
		color->r = (uint8_t)c_round(r);
		color->g = (uint8_t)c_round(g);
		color->b = (uint8_t)c_round(b);
		color->a = (uint8_t)c_round(a);
	}
	else {
		*color = *back;
	}
}

void PiuColorDictionary(xsMachine *the, xsSlot* slot, PiuColor color)
{
	xsType type = xsTypeOf(*slot);
	if ((type == xsIntegerType) || (type == xsNumberType)) {
		uint32_t value = fxToUnsigned(the, slot);
		color->r = (value >> 24) & 0xFF;
		color->g = (value >> 16) & 0xFF;
		color->b = (value >> 8) & 0xFF;
		color->a = value & 0xFF;
	}
	else {
		xsStringValue s = xsToString(*slot);
		if ('#' == c_read8(s)) {
			xsIntegerValue length = c_strlen(s);
			if (length == 4) {
				color->r = (uint8_t)PiuStringHexToNum(&s[1], 1) * 17;
				color->g = (uint8_t)PiuStringHexToNum(&s[2], 1) * 17;
				color->b = (uint8_t)PiuStringHexToNum(&s[3], 1) * 17;
				color->a = 255;
				return;
			}
			if (length == 5) {
				color->r = (uint8_t)PiuStringHexToNum(&s[1], 1) * 17;
				color->g = (uint8_t)PiuStringHexToNum(&s[2], 1) * 17;
				color->b = (uint8_t)PiuStringHexToNum(&s[3], 1) * 17;
				color->a = (uint8_t)PiuStringHexToNum(&s[1], 1) * 17;
				return;
			}
			if (length == 7) {
				color->r = (uint8_t)PiuStringHexToNum(&s[1], 2);
				color->g = (uint8_t)PiuStringHexToNum(&s[3], 2);
				color->b = (uint8_t)PiuStringHexToNum(&s[5], 2);
				color->a = 255;
				return;
			}
			if (length == 9) {
				color->r = (uint8_t)PiuStringHexToNum(&s[1], 2);
				color->g = (uint8_t)PiuStringHexToNum(&s[3], 2);
				color->b = (uint8_t)PiuStringHexToNum(&s[5], 2);
				color->a = (uint8_t)PiuStringHexToNum(&s[7], 2);
				return;
			}
		}
		else {
			xsIntegerValue i;
			for (i = 0; i < PiuColorCount; i++) {
				if (!c_strcmp(s, PiuColorNames[i])) {
					color->r = c_read8(&PiuColorValues[i].r);
					color->g = c_read8(&PiuColorValues[i].g);
					color->b = c_read8(&PiuColorValues[i].b);
					color->a = c_read8(&PiuColorValues[i].a);
					return;
				}
			}
		}
		xsErrorPrintf("invalid color");
	}
}

void PiuColorsDictionary(xsMachine *the, xsSlot* slot, PiuColor colors)
{
	if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
		xsIntegerValue i, c = xsToInteger(xsGet(*slot, xsID_length));
		if (c > 4) c = 4;
		for (i = 0; i < c; i++) {
			xsVar(0) = xsGetAt(*slot, xsInteger(i));
			PiuColorDictionary(the, &xsVar(0), &colors[i]);
		}
	}
	else {
		PiuColorRecord color;
		PiuColorDictionary(the, slot, &color);
		colors[0] = color;
		colors[1] = color;
		colors[2] = color;
		colors[3] = color;
	}
}

void PiuColorsSerialize(xsMachine *the, PiuColor colors)
{
	xsIntegerValue i;
	xsResult = xsNew1(xsGlobal, xsID_Array, xsInteger(4));
	(void)xsCall0(xsResult, xsID_fill);
	for (i = 0; i < 4; i++) {
		PiuColor color = &colors[i];
		fxUnsigned(the, &xsVar(0), ((uint32_t)color->r << 24) | ((uint32_t)color->g << 16) | ((uint32_t)color->b << 8) | ((uint32_t)color->a));
		xsSetAt(xsResult, xsInteger(i), xsVar(0));
	}
}

void PiuHSL2RGB(const uint8_t *hsl, uint8_t *rgb)
{
	int v;

	v = (hsl[2] <= 255/2)	? (((int)hsl[2] * (255 + (int)hsl[1])) + (255 >> 1)) / 255
							: (int)hsl[2] + (int)hsl[1] - ((int)hsl[2] * (int)hsl[1] + (255 >> 1)) / 255;
	if (v <= 0) {
		rgb[0] = rgb[1] = rgb[2] = 0;
	}
	else {
		int m, sv, h, fract, vsf, mid1, mid2, sextant;
		m = (int)hsl[2] + (int)hsl[2] - v;
		sv = (((v - m) << 8) + (v >> 1)) / v;	/* 24.8: [0, 255/256] */
		h = (int)hsl[0] * 6 * 257;				/* 16.16 */
		h = (h + (1 << (8 - 1))) >> 8;			/* 24.8: [0, 5 + 255/256] */
		sextant = h >> 8;
		if (sextant > 5)
			sextant -= 6;
		fract = h & 0xFF;						/* 24.8: [0, 255/256] */
		vsf = (v * sv * fract + (1 << (16 - 1))) >> 16;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant) {
			case 0:		rgb[0] = v;		rgb[1] = mid1;	rgb[2] = m;		break;
			case 1:		rgb[0] = mid2;	rgb[1] = v;		rgb[2] = m;		break;
			case 2:		rgb[0] = m;		rgb[1] = v;		rgb[2] = mid1;	break;
			case 3:		rgb[0] = m;		rgb[1] = mid2;	rgb[2] = v;		break;
			case 4:		rgb[0] = mid1;	rgb[1] = m;		rgb[2] = v;		break;
			case 5:		rgb[0] = v;		rgb[1] = m;		rgb[2] = mid2;	break;
		}
	}
}

xsIntegerValue PiuMinMax(xsIntegerValue min, xsIntegerValue val, xsIntegerValue max)
{
	if (val < min) val = min;
	else if (max < val) val = max;
	return val;
}

uint32_t PiuStringHexToNum(const char *string, uint32_t i)
{
	uint32_t v = 0;
	while (i) {
		char c = c_read8(string++);
		if (('0' <= c) && (c <= '9'))
			v = (v << 4) + (c - '0');
		else if (('a' <= c) && (c <= 'f'))
			v = (v << 4) + (10 + c - 'a');
		else if (('A' <= c) && (c <= 'F'))
			v = (v << 4) + (10 + c - 'A');
		else
			break;
		i--;		
	}
	return v;
}

void Piu_blendColors(xsMachine* the)
{
	PiuColorRecord c0;
	PiuColorRecord c1;
	PiuColorRecord c2;
    xsNumberValue plus = xsToNumber(xsArg(0));
	xsNumberValue minus = 1 - plus;
	PiuColorDictionary(the, &xsArg(1), &c1);
	PiuColorDictionary(the, &xsArg(2), &c2);
	c0.r = (uint8_t)c_round(((double)c1.r * minus) + ((double)c2.r * plus));
	c0.g = (uint8_t)c_round(((double)c1.g * minus) + ((double)c2.g * plus));
	c0.b = (uint8_t)c_round(((double)c1.b * minus) + ((double)c2.b * plus));
	c0.a = (uint8_t)c_round(((double)c1.a * minus) + ((double)c2.a * plus));
	fxUnsigned(the, &xsResult, ((uint32_t)c0.r << 24) | ((uint32_t)c0.g << 16) | ((uint32_t)c0.b << 8) | ((uint32_t)c0.a));
}

void Piu_hsl(xsMachine* the)
{
	uint8_t hsl[3];
	PiuColorRecord color;
	xsNumberValue value = c_fmod(xsToNumber(xsArg(0)), 360.f);
	if (value < 0) value += 360.f;
	hsl[0] = (uint8_t)c_round(value * 255.f / 360.f);
	hsl[1] = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(1)) * 255.f), 255);
	hsl[2] = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(2)) * 255.f), 255);
	PiuHSL2RGB(hsl, (uint8_t*)&color);
    color.a = 255;
	fxUnsigned(the, &xsResult, ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | ((uint32_t)color.a));
}

void Piu_hsla(xsMachine* the)
{
	uint8_t hsl[3];
	PiuColorRecord color;
	xsNumberValue value = c_fmod(xsToNumber(xsArg(0)), 360.f);
	if (value < 0)  value += 360.f;
	hsl[0] = (uint8_t)c_round(value * 255.f / 360.f);
	hsl[1] = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(1)) * 255.f), 255);
	hsl[2] = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(2)) * 255.f), 255);
	PiuHSL2RGB(hsl, (uint8_t*)&color);
    color.a = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(3)) * 255.f), 255);
	fxUnsigned(the, &xsResult, ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | ((uint32_t)color.a));
}

void Piu_rgb(xsMachine* the)
{
	PiuColorRecord color;
	color.r = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(0)), 255);
	color.g = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(1)), 255);
	color.b = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(2)), 255);
	color.a = 255;
	fxUnsigned(the, &xsResult, ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | ((uint32_t)color.a));
}

void Piu_rgba(xsMachine* the)
{
	PiuColorRecord color;
	color.r = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(0)), 255);
	color.g = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(1)), 255);
	color.b = (uint8_t)PiuMinMax(0, xsToInteger(xsArg(2)), 255);
	color.a = (uint8_t)PiuMinMax(0, (xsIntegerValue)c_round(xsToNumber(xsArg(3)) * 255.f), 255);
	fxUnsigned(the, &xsResult, ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | ((uint32_t)color.a));
}



