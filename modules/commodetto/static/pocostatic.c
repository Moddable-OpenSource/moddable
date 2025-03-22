/*
* Copyright (c) 2021-2024  Moddable Tech, Inc.
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
#include "commodettoPoco.h"
#include "commodettoPocoBlit.h"

#if ESP32
	// c_rand() defined to esp_random() which is cryptographically secure but too slow....
	#define RANDOM rand
#else
	#define RANDOM c_rand
#endif
/*
	state for one blit operation, stored in the display list

	for static blitter, it stores a ramp of 16 color values
*/
typedef struct {
	PocoPixel colors[16];
} PocoStaticRecord, *PocoStatic;

/*
	blitter. called from top to bottom to render the pixels
*/
static void doStatic(Poco poco, uint8_t *refcon, PocoPixel *dst, PocoDimension w, PocoDimension h, uint8_t xphase)
{
	PocoStatic sr = (PocoStatic)refcon;
	PocoDimension rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - w;
	unsigned int r = 0;

	while (h--) {
		PocoDimension tw = w;
		while (tw--) {
			if (!r)
				r = RANDOM() & 0x0fffffff;
			*dst++ = sr->colors[r & 0x0F];
			r >>= 4;
		}
		dst += rowBump;
	}
}

/*
	C function to set-up the the blitter state for one blit operation
*/
static void PocoFillStatic(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, int brightness)
{
	PocoStaticRecord sr;
	PocoCoordinate xMax, yMax;
	uint8_t c;

	doPocoBlitterClip(x, y, w, h, xMax, yMax);

	for (c = 0; c < 16; c++) {
		int cmp = (((c << 4) | c) * brightness) >> 8;
		if (cmp < 0) cmp = 0;
		if (cmp > 255) cmp = 255;
		sr.colors[c] = PocoMakeColor(poco, cmp, cmp, cmp);
	}

	PocoDrawExternal(poco, doStatic, (void *)&sr, sizeof(sr), x, y, w, h);
}

/*
	Binds JavaScript call to native C function
*/
void xs_poco_fillStatic(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	PocoCoordinate x = (PocoCoordinate)xsmcToInteger(xsArg(0)) + poco->xOrigin;
	PocoCoordinate y = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->yOrigin;
	PocoDimension w = (PocoDimension)xsmcToInteger(xsArg(2));
	PocoDimension h = (PocoDimension)xsmcToInteger(xsArg(3));
	int brightness = (xsmcArgc > 4) ? xsmcToInteger(xsArg(4)) : 255;

	PocoFillStatic(poco, x, y, w, h, brightness);
}
