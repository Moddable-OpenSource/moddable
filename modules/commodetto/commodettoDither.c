/*
 * Copyright (c) 2017-2021  Moddable Tech, Inc.
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
 
#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "commodettoPocoBlit.h"

/*
		Dither to 1-bit (monochrome) destination  

		only 8 bit gray source pixels supported
*/

#if kPocoPixelFormat != kCommodettoBitmapGray256
	#error unsupported pixel format
#endif

typedef void (*ditherBlit)(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors);

#define getErrorBufferLength(width) (width + 4)

struct DitherRecord {
	ditherBlit	blit;
	uint16_t	width;
	uint8_t		mode;
	uint8_t		phase;
	
	int16_t		errors[1];
};
typedef struct DitherRecord DitherRecord;
typedef struct DitherRecord *Dither;

static void commodettoDitherNone(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors);
static void commodettoDitherAtkinson(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors);
static void commodettoDitherBurkes(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors);

void xs_dither(xsMachine *the)
{
	DitherRecord dither;
	int width;
	
	xsmcVars(1);
	
	xsmcGet(xsVar(0), xsArg(0), xsID_width);
	width = xsmcToInteger(xsVar(0));
	if ((width <= 0) || (width > 65535) || (width & 7))
		xsUnknownError("invalid");
	dither.width = (uint16_t)width;

	dither.blit = commodettoDitherAtkinson;
	xsmcGet(xsVar(0), xsArg(0), xsID_algorithm);
	if (xsUndefinedType != xsmcTypeOf(xsVar(0))) { 
		char *ditherName = xsmcToString(xsVar(0));
		if (!c_strcmp(ditherName, "none"))
			dither.blit = commodettoDitherNone;
		else if (!c_strcmp(ditherName, "atkinson"))
			dither.blit = commodettoDitherAtkinson;
		else if (!c_strcmp(ditherName, "burkes"))
			dither.blit = commodettoDitherBurkes;
		else
			xsUnknownError("invalid");
	}

	xsmcSetHostChunk(xsThis, &dither, sizeof(dither) + (getErrorBufferLength(width) * sizeof(int16_t) * 2));
}

void xs_dither_destructor(void *data)
{
}

void xs_dither_close(xsMachine *the)
{
	Dither dither = xsmcGetHostChunk(xsThis);
	xsmcSetHostData(xsThis, NULL);
	xs_dither_destructor(dither);
}

void xs_dither_reset(xsMachine *the)
{
	Dither dither = xsmcGetHostChunk(xsThis);

	c_memset(dither->errors, 0, getErrorBufferLength(dither->width) * sizeof(int16_t) * 2);
	dither->phase = 0;
}

void xs_dither_send(xsMachine *the)
{
	int lines = xsmcToInteger(xsArg(0));
	int srcOffset = xsmcToInteger(xsArg(2));
	int dstOffset = xsmcToInteger(xsArg(4));
	uint8_t *src, *dst;
	xsUnsignedValue srcLength, dstLength, pixelCount;

	xsmcGetBufferReadable(xsArg(1), (void **)&src, &srcLength);
	xsmcGetBufferWritable(xsArg(3), (void **)&dst, &dstLength);

	src += srcOffset;
	dst += dstOffset;

	//@@ range check on offset and lines

	Dither dither = xsmcGetHostChunk(xsThis);
	uint16_t width = dither->width;
	int16_t *ditherA = dither->errors + 2;
	int16_t *ditherB = dither->errors + getErrorBufferLength(width) + 2;

	while (lines--) {
		if (dither->phase) {
			(dither->blit)(src, dst, width, ditherA, ditherB);
			dither->phase = 0;
		}
		else { 
			(dither->blit)(src, dst, width, ditherB, ditherA);
			dither->phase = 1;
		}

		src += width;
		dst += width >> 3;
	}
}

void commodettoDitherNone(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors)
{
	uint8_t pixelMask = 0x80, pixels = 0;

	do {
		if (*src++ >= 128)
			pixels |= pixelMask;

		pixelMask >>= 1;
		if (0 == pixelMask) {
			*dst++ = pixels;
			pixelMask = 0x80;
			pixels = 0;
		}
	} while (--width);
}

void commodettoDitherAtkinson(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors)
{
	uint8_t pixelMask = 0x80, pixels = 0;

	do {
		int16_t thisPixel = *src++ + (thisLineErrors[0] >> 3);

		if (thisPixel >= 128) {
			pixels |= pixelMask;
			thisPixel -= 255;
		}

		thisLineErrors[ 0]  = thisPixel;		// next next!
		thisLineErrors[+1] += thisPixel;
		thisLineErrors[+2] += thisPixel;

		nextLineErrors[-1] += thisPixel;
		nextLineErrors[ 0] += thisPixel;
		nextLineErrors[+1] += thisPixel;

		thisLineErrors++;
		nextLineErrors++;

		pixelMask >>= 1;
		if (0 == pixelMask) {
			*dst++ = pixels;
			pixelMask = 0x80;
			pixels = 0;
		}
	} while (--width);
}

void commodettoDitherBurkes(uint8_t *src, uint8_t *dst, uint16_t width, int16_t *thisLineErrors, int16_t *nextLineErrors)
{
	uint8_t pixelMask = 0x80, pixels = 0;
	c_memset(nextLineErrors - 2, 0, getErrorBufferLength(width) * sizeof(int16_t));

	do {
		int16_t thisPixel = *src++ + (thisLineErrors[0] >> 4);

		if (thisPixel >= 128) {
			pixels |= pixelMask;
			thisPixel -= 255;
		}

		thisLineErrors[ 1] += thisPixel << 2;
		thisLineErrors[ 2] += thisPixel << 1;
		nextLineErrors[-2] += thisPixel;
		nextLineErrors[-1] += thisPixel << 1;
		nextLineErrors[ 0] += thisPixel << 2;
		nextLineErrors[ 1] += thisPixel << 1;
		nextLineErrors[ 2] += thisPixel;

		thisLineErrors++;
		nextLineErrors++;

		pixelMask >>= 1;
		if (0 == pixelMask) {
			*dst++ = pixels;
			pixelMask = 0x80;
			pixels = 0;
		}
	} while (--width);
}
