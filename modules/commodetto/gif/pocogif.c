/*
* Copyright (c) 2021  Moddable Tech, Inc.
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
#include "commodettoPoco.h"

#if (0 != kPocoRotation)
	#pragma error rotation unimplemented
#endif

typedef struct {
	PocoPixel 	*pixels;
	uint16_t	rowPixels;
	PocoPixel	keyColor;
} GIFRecord, *GIF;

static void doGIF(Poco poco, uint8_t *refcon, PocoPixel *dst, PocoDimension w, PocoDimension h);

void xs_poco_drawGIF(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	GIFRecord gif;
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	int16_t d;
	CommodettoBitmap cb;
	PocoPixel *pixels;
	PocoPixel keyColor = (PocoPixel)xsmcToInteger(xsArg(3));

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;

	cb = xsmcGetHostChunk(xsArg(0));
	if (!cb->havePointer)
		xsUnknownError("unsupported");

	pixels = (PocoPixel *)cb->bits.data;
	sx = 0, sy = 0;
	sw = cb->w, sh = cb->h;

	if ((x >= poco->xMax) || (y >= poco->yMax))
		return;

	if (x < poco->x) {
		d = poco->x - x;
		if (sw <= d)
			return;
		sx += d;
		sw -= d;
		x = poco->x;
	}

	if (y < poco->y) {
		d = poco->y - y;
		if (sh <= d)
			return;
		sy += d;
		sh -= d;
		y = poco->y;
	}

	if ((x + sw) > poco->xMax)
		sw = poco->xMax - x;

	if ((y + sh) > poco->yMax)
		sh = poco->yMax - y;

	if ((sx >= cb->w) || (sy >= cb->h) || ((sx + sw) > cb->w) || ((sy + sh) > cb->h) || !sw || !sh)
		return;

	pixels += (sy * cb->w) + sx;
	gif.pixels = pixels;
	gif.rowPixels = cb->w;
	gif.keyColor = keyColor;

	PocoDrawExternal(poco, doGIF, (void *)&gif, sizeof(gif), x, y, sw, sh);
}

void doGIF(Poco poco, uint8_t *refcon, PocoPixel *dst, PocoDimension w, PocoDimension h)
{
	GIF gif = (GIF)refcon;
	PocoPixel *src = gif->pixels;
	PocoCoordinate srcRowPixels = gif->rowPixels;
	PocoPixel keyColor = gif->keyColor;

	do {
		PocoPixel *s = src, *d = dst;
		int count = w;
		do {
			PocoPixel pixel = *s++;
			if (keyColor != pixel)
				*d = pixel;
			d++;
		} while (--count);

		dst = (PocoPixel *)(poco->rowBytes + (char *)dst);
		src += srcRowPixels;
	} while (--h);

	gif->pixels = src;
}
