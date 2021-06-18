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
#include "mc.xs.h"      // for xsID_ values
#include "commodettoPoco.h"

#if (0 != kPocoRotation)
	#pragma error rotation unimplemented
#endif

typedef struct {
	uint8_t 		*pixels;
	uint8_t			size;
	uint8_t			scale;
	PocoDimension	sx;
	PocoDimension	sy;
	PocoDimension	sw;
	PocoPixel		fore;
} QRCodeRecord, *QRCode;

static void doQRCode(Poco poco, uint8_t *refcon, PocoPixel *dst, PocoDimension w, PocoDimension h, uint8_t xphase);

//function drawQRCode(bits, x, y, fore) @ "xs_poco_drawQRCode";

void xs_poco_drawQRCode(xsMachine *the)
{
	Poco poco = xsmcGetHostDataPoco(xsThis);
	PocoCoordinate x, y;
	PocoDimension sx, sy, sw, sh;
	int16_t d;
	QRCodeRecord qr;
	PocoDimension side;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_size);
	qr.size = (uint8_t)xsmcToInteger(xsVar(0));

	x = (PocoCoordinate)xsmcToInteger(xsArg(1)) + poco->xOrigin;
	y = (PocoCoordinate)xsmcToInteger(xsArg(2)) + poco->yOrigin;
	qr.scale = (uint8_t)xsmcToInteger(xsArg(3));
	qr.fore = (PocoPixel)xsmcToInteger(xsArg(4));

	side = qr.size * qr.scale;

	sx = sy = 0;
	sw = sh = side;

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

	if ((sx >= side) || (sy >= side) || ((sx + sw) > side) || ((sy + sh) > side) || !sw || !sh)
		return;

	qr.sy = sy;
	qr.sx = sx;
	qr.sw = sw;
	qr.pixels = xsmcToArrayBuffer(xsArg(0));
	PocoDisableGC(poco);

	PocoDrawExternal(poco, doQRCode, (void *)&qr, sizeof(qr), x, y, sw, sh);
}

void doQRCode(Poco poco, uint8_t *refcon, PocoPixel *dst, PocoDimension w, PocoDimension h, uint8_t xphase)
{
	QRCode qr = (QRCode)refcon;

	do {
		uint8_t *s = qr->pixels + ((qr->sy / qr->scale) * qr->size) + (qr->sx / qr->scale);
		PocoDimension dx = qr->scale - (qr->sx % qr->scale);
		PocoPixel *d = dst;
		int count = w;
		do {
			if (*s)
				*d = qr->fore;
			d++;

			if (0 == --dx) {
				dx = qr->scale;
				s++;
			}
		} while (--count);

		dst = (PocoPixel *)(poco->rowBytes + (char *)dst);
		qr->sy += 1;
	} while (--h);
}
