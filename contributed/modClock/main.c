/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

#include "xsmc.h"
#include "xsHost.h"
#include "buildinfo.h"

void xs_getbuildstring(xsMachine *the)
{
	int len;
	char *str;

	len = c_strlen(_BuildInfo.date) + c_strlen(_BuildInfo.time)
		+ c_strlen(_BuildInfo.src_version) + c_strlen(_BuildInfo.env_version);
	len += 4;
	str = c_malloc(len);
	c_strcpy(str, _BuildInfo.date);
	c_strcat(str, " ");
	c_strcat(str, _BuildInfo.time);
	c_strcat(str, " ");
	c_strcat(str, _BuildInfo.src_version);
	c_strcat(str, " ");
	c_strcat(str, _BuildInfo.env_version);
	xsmcSetString(xsResult, str);
}

void do_restart(xsMachine *the)
{
	esp_restart();
}

void xs_hsvtorgb(xsMachine *the)
{
	double h, s, v, r, g, b;

	h = xsmcToNumber(xsArg(0));
	s = xsmcToNumber(xsArg(1));
	v = xsmcToNumber(xsArg(2));

	int i = h * 6;
	double f = h * 6 - i;
	double p = v * (1 - s);
	double q = v * (1 - f * s);
	double t = v * (1 - (1 - f) * s);

	switch (i) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
	}

	i = (((int)(r * 255) & 0xff) << 16) | (((int)(g * 255) & 0xff) << 8) | ((int)(b * 255) & 0xff);
	xsmcSetInteger(xsResult, i);
}

void xs_mergeAndScale(xsMachine *the)
{
	uint32_t color1, color2;
	double amt;
	int ar, ag, ab;
	int br, bg, bb;

	amt = xsmcToNumber(xsArg(0));
	color1 = xsmcToInteger(xsArg(1));
	color2 = xsmcToInteger(xsArg(2));

	ar = (color1 & 0xff0000) * amt;
	ag = (color1 & 0x00ff00) * amt;
	ab = (color1 & 0x0000ff) * amt;
	amt = 1 - amt;
	br = (color2 & 0xff0000) * amt;
	bg = (color2 & 0x00ff00) * amt;
	bb = (color2 & 0x0000ff) * amt;

	ar = ar + br;
	ag = ag + bg;
	ab = ab + bb;
	if (ar > 0xff0000) ar = 0xff0000;
	if (ag > 0x00ff00) ag = 0x00ff00;
	if (ab > 0x0000ff) ab = 0x0000ff;

	xsmcSetInteger(xsResult, (ar&0xff0000) | (ag&0x00ff00) | (ab&0x0000ff));
}

uint32_t scaleColor(uint32_t c, double amt) {
	uint32_t r, g, b;
	r = ((c & 0xff0000) >> 16) * amt;
	g = ((c & 0x00ff00) >>  8) * amt;
	b =  (c & 0x0000ff)        * amt;
	return ((r&0xff) << 16 | (g&0xff) << 8 | (b&0xff));
}

uint32_t dimColor(uint32_t c, uint8_t amt) {
	uint32_t r, g, b;
	r = (((c & 0xff0000) >> 16) - amt);
	if (r > 0xff) r = 0;
	g = (((c & 0x00ff00) >>  8) - amt);
	if (g > 0xff) g = 0;
	b = ( (c & 0x0000ff)        - amt);
	if (b > 0xff) b = 0;
	return (r << 16 | g << 8 | b);
}

void xs_scaleColor(xsMachine *the)
{
	uint32_t color;
	double amt;
	color = xsmcToInteger(xsArg(0));
	amt = xsmcToNumber(xsArg(1));
	xsmcSetInteger(xsResult, scaleColor(color, amt));
}

void xs_dimColor(xsMachine *the)
{
	uint32_t color, amt;
	color = xsmcToInteger(xsArg(0));
	amt = xsmcToInteger(xsArg(1));
	xsmcSetInteger(xsResult, dimColor(color, amt));
}

void xs_raise(xsMachine *the)
{
	uint32_t pix, color, amt;
}

void xs_brightenAndConvert(xsMachine *the)
{
	uint32_t color, brightness, out;
	int r, g, b;
	char *order;

	color = xsmcToInteger(xsArg(0));
	brightness = xsmcToInteger(xsArg(1));
	order = xsmcToString(xsArg(2));

	r = ((color & 0xff0000) * brightness) >> 24;
	g = ((color & 0x00ff00) * brightness) >> 16;
	b = ((color & 0x0000ff) * brightness) >> 8;

	if (0 == c_strcmp(order, "RGB"))
		out = (r << 16) | (g << 8) | b;
	else if (0 == c_strcmp(order, "GRB"))
		out = (g << 16) | (r << 8) | b;
	else if (0 == c_strcmp(order, "RGBW"))
		out = (r << 24) | (g << 16) | (b << 8);
	else
		xsErrorPrintf("Unknown LED order");

	xsmcSetInteger(xsResult, out);
}

void xs_hueDist(xsMachine *the)
{
	double x, y, span, r;

	x = xsmcToNumber(xsArg(0));
	y = xsmcToNumber(xsArg(1));
	span = xsmcToNumber(xsArg(2));
	r = c_sqrt((x*x)+(y*y)) / span;
	xsmcSetNumber(xsResult, r);
}
