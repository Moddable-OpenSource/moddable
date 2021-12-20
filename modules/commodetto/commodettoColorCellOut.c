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


#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#include "commodettoBitmap.h"
#include "commodettoConvert.h"
#include "commodettoPocoBlit.h"

typedef struct {
	PocoPixel			prevLight;
	PocoPixel			prevDark;

	uint16_t			outputOffset;
	uint16_t			outputMax;
	uint16_t			quality;

	CommodettoDimension	width;
	CommodettoDimension	height;

	uint8_t				output[1];
} ColorCellOutRecord, *ColorCellOut;

static void outputByte(xsMachine *the, ColorCellOut cc, uint8_t value);
static void outputUint16(xsMachine *the, ColorCellOut cc, uint16_t value);
static void outputUint32(xsMachine *the, ColorCellOut cc, uint32_t value);
static void outputPixel(xsMachine *the, ColorCellOut cc, PocoPixel pixel);

static void encodeRow(xsMachine *the, ColorCellOut cc, PocoPixel *pixels);

void xs_colorcell_destructor(void *data)
{
}

void xs_colorcell(xsMachine *the)
{
}

void xs_colorcell_begin(xsMachine *the)
{
	ColorCellOut cc;
	int i, w, h, outputMax, quality = 1024 / 2;

	xsmcVars(1);

	i = xsmcToInteger(xsArg(0));
	if (i)
		xsUnknownError("x must be 0");

	i = xsmcToInteger(xsArg(1));
	if (i)
		xsUnknownError("y must be 0");

	w = xsmcToInteger(xsArg(2));
	if ((w < 4) || (w & 3))
		xsUnknownError("width must be multiple of 4");

	h = xsmcToInteger(xsArg(3));
	if ((h < 4) || (h & 3))
		xsUnknownError("height must be multiple of 4");

	if (xsmcArgc >= 5) {
		xsmcGet(xsVar(0), xsArg(4), xsID_quality);
		quality	= (int)(xsmcToNumber(xsVar(0)) * 1023);
		if (quality < 0) quality = 0;
		if (quality > 1023) quality = 1023;
	}

	outputMax = (w >> 2) * (h >> 2) * 16;		// estimate 16 bytes per cell... this should vary based on quality
	outputMax += 16;							// space for header
	if (outputMax > 65535)
		outputMax = 65535;
	cc = xsmcSetHostChunk(xsThis, NULL, sizeof(ColorCellOutRecord) + outputMax);

	cc->width = (CommodettoDimension)w;
	cc->height = (CommodettoDimension)h;

	cc->outputOffset = 0;
	cc->outputMax = outputMax;

	cc->prevLight = PocoMakePixelRGB565LE(255, 255, 255);
	cc->prevDark = PocoMakePixelRGB565LE(0, 0, 0);

	cc->quality = quality;
}

void xs_colorcell_send(xsMachine *the)
{
	ColorCellOut cc;
	int argc = xsmcArgc;
	const uint8_t *data;
	int rows;
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &count);

	if (argc > 1) {
		int offset = xsmcToInteger(xsArg(1));

		if ((offset < 0) || ((xsUnsignedValue)offset >= count))
			xsUnknownError("bad offset");
		data += offset;
		count -= offset;
		if (argc > 2) {
			int c = xsmcToInteger(xsArg(2));
			if ((c < 0) || ((xsUnsignedValue)c >= count))
				xsUnknownError("bad count");
			count = c;
		}
	}

	cc = xsmcGetHostChunk(xsThis);

	if (count % (cc->width * 4 * 2))
		xsUnknownError("must send multiple of 4 rows");

	rows = count / (cc->width * 4 * 2);
	while (rows--) {
		encodeRow(the, cc, (PocoPixel *)data);
		data += (cc->width * 4 * 2);
	}
}

void xs_colorcell_end(xsMachine *the)
{
	ColorCellOut cc = xsmcGetHostChunk(xsThis);

	xsmcVars(1);

	xsmcSetArrayBuffer(xsVar(0), NULL, cc->outputOffset);
	cc = xsmcGetHostChunk(xsThis);
	c_memmove(xsmcToArrayBuffer(xsVar(0)), cc->output, cc->outputOffset);
	xsmcSetHostChunk(xsThis, NULL, 0);

	xsCall4(xsThis, xsID_build, xsInteger(cc->width), xsInteger(cc->height), xsInteger(kCommodettoBitmapColorCell), xsVar(0));
}

void outputByte(xsMachine *the, ColorCellOut cc, uint8_t value)
{
	if (cc->outputOffset >= cc->outputMax)
		xsUnknownError("output overflow");

	cc->output[cc->outputOffset] = value;
	cc->outputOffset += 1;
}

void outputUint16(xsMachine *the, ColorCellOut cc, uint16_t value)
{
	outputByte(the, cc, value & 255);
	outputByte(the, cc, value >> 8);
}

void outputUint32(xsMachine *the, ColorCellOut cc, uint32_t value)
{
	outputByte(the, cc, value & 255);
	outputByte(the, cc, value >> 8);
	outputByte(the, cc, value >> 16);
	outputByte(the, cc, value >> 24);
}

void outputPixel(xsMachine *the, ColorCellOut cc, PocoPixel pixel)
{
	outputByte(the, cc, pixel & 255);
	outputByte(the, cc, pixel >> 8);
}

static uint32_t colorDelta(PocoPixel a, PocoPixel b);
static void pixelToRGB(PocoPixel pixel, uint8_t *r, uint8_t *g, uint8_t *b);
static PocoPixel rgbToPixel(uint8_t r, uint8_t g, uint8_t b);

#define kReuse0Mask (0x10)
#define kReuse1Mask (0x08)

#define kThresholdColorReuse (0)

#define kErrorMax (0xffffffff)

void encodeRow(xsMachine *the, ColorCellOut cc, PocoPixel *pixels)
{
	int x, i;
	CommodettoConverter convert = CommodettoPixelsConverterGet(kCommodettoBitmapRGB565LE, kCommodettoBitmapGray256);
	uint32_t thresholdOneColor = kErrorMax, thresholdTwoColor = kErrorMax, thresholdFourColor = kErrorMax;
	uint8_t previousIsSolid = 0;
	PocoPixel previousSolidColor = 0;
	uint16_t previousSolidOffset = 0;

	if (cc->quality < 333) {
		if (cc->quality)
			thresholdOneColor = 128 + (((333 - cc->quality) * 1024) / 333);		// from 1152 to 128
	}
	else if (cc->quality < 667) {
		thresholdOneColor = 32 + (((667 - cc->quality) * 96) / 333);			// from 128 to 32
		thresholdTwoColor = 64 + (((667 - cc->quality) * 1088) / 333);			// from 1152 to 64
	}
	else {
		thresholdOneColor = 16 + (((1023 - cc->quality) * 16) / 333);			// from 32 to 16
		thresholdTwoColor = 64 + (((1023 - cc->quality) * 64) / 333);			// from 128 to 64
		thresholdFourColor = 128 + (((1023 - cc->quality) * 1024) / 333);		// from 1152 to 128
	}

	for (x = 0; x < cc->width; x += 4, pixels += 4) {
		PocoPixel cell[16];
		uint8_t luma[16];
		uint32_t lumaAverage, rAverage, gAverage, bAverage;
		uint8_t command;
		PocoPixel twoColorLight = 0, twoColorDark = 0, colorAverage;
		PocoPixel fourColorLight[3], fourColorDark[3];
		uint16_t twoColorBitmap;
		uint16_t twoColorCountLight, twoColorCountDark;
		uint32_t fourColorBitmap[3];
		uint8_t r, g, b;
		uint32_t errorOneColor = kErrorMax, errorTwoColor = kErrorMax, errorFourColor = kErrorMax;
		uint8_t bestPass = 0;

		// copy pixels in cell to local array
		c_memcpy(&cell[ 0], pixels + (0 * cc->width), 4 * sizeof(PocoPixel));
		c_memcpy(&cell[ 4], pixels + (1 * cc->width), 4 * sizeof(PocoPixel));
		c_memcpy(&cell[ 8], pixels + (2 * cc->width), 4 * sizeof(PocoPixel));
		c_memcpy(&cell[12], pixels + (3 * cc->width), 4 * sizeof(PocoPixel));

		/*
			One color
		*/

		// calculate average color
		for (i = 0, rAverage = 8, gAverage = 8, bAverage = 8; i < 16; i++) {
			pixelToRGB(cell[i], &r, &g, &b);
			rAverage += r, gAverage += g, bAverage += b;
		}
		colorAverage = rgbToPixel(rAverage >> 4, gAverage >> 4, bAverage >> 4);

		for (i = 0, errorOneColor = 0; i < 16; i++)
			errorOneColor += colorDelta(cell[i], colorAverage);

		/*
			Two colors
		*/

		// calculate luma for all pixels
		(convert)(16, cell, luma, NULL);

		// calculate average luma
		for (i = 0, lumaAverage = 8; i < 16; i++)
			lumaAverage += luma[i];
		lumaAverage >>= 4;

		// calculate two representative colors by partitioning pixels based on average luma, then averaging the colors in those two groups
		for (i = 0, twoColorCountLight = 0, rAverage = 0, gAverage = 0, bAverage = 0; i < 16; i++) {
			if (luma[i] > lumaAverage) {
				pixelToRGB(cell[i], &r, &g, &b);
				rAverage += r, gAverage += g, bAverage += b;
				twoColorCountLight += 1;
			}
		}
		twoColorCountDark = 16 - twoColorCountLight;
		if (twoColorCountLight)
			twoColorLight = rgbToPixel((rAverage + (twoColorCountLight >> 1)) / twoColorCountLight, (gAverage + (twoColorCountLight >> 1)) / twoColorCountLight, (bAverage + (twoColorCountLight >> 1)) / twoColorCountLight);

		for (i = 0, rAverage = 0, gAverage = 0, bAverage = 0; i < 16; i++) {
			if (luma[i] <= lumaAverage) {
				pixelToRGB(cell[i], &r, &g, &b);
				rAverage += r, gAverage += g, bAverage += b;
			}
		}
		if (twoColorCountDark)
			twoColorDark = rgbToPixel((rAverage + (twoColorCountDark >> 1)) / twoColorCountDark, (gAverage + (twoColorCountDark >> 1)) / twoColorCountDark, (bAverage + (twoColorCountDark >> 1)) / twoColorCountDark);

		// assign pixels to two selected colors
		{
			for (i = 0, twoColorBitmap = 0, twoColorCountLight = 0, twoColorCountDark = 0, errorTwoColor = 0; i < 16; i++) {
				uint32_t errorDark = colorDelta(cell[i], twoColorDark), errorLight = colorDelta(cell[i], twoColorLight);
				if (errorDark < errorLight) {
					twoColorBitmap |= 1 << (15 - i);
					errorTwoColor += errorDark;
					twoColorCountDark += 1;
				}
				else {
					errorTwoColor += errorLight;
					twoColorCountLight += 1;
				}
			}
			// if only used one color, force ouptut to single color
			if (0 == twoColorBitmap)
				twoColorCountDark = 0, twoColorCountLight = 16;
			else if (0xFFFF == twoColorBitmap)
				twoColorCountLight = 0, twoColorCountDark = 16;
		}
		fourColorLight[0] = twoColorCountLight;
		fourColorDark[0] = twoColorCountDark;

		/*
			Four colors
		*/
		// calculate two alternate representative colors by finding the two colors the greatest distance from each other
		if (1) {
		PocoPixel fourColors[4];
		uint8_t rl, gl, bl;
		uint8_t rd, gd, bd;
		PocoPixel farthest0, farthest1;
		int bestError = -1;
		uint8_t pass;
		uint32_t bestPassError = kErrorMax;

		for (i = 0; i < 16; i++) {
			int j;

			for (j = i + 1; j < 16; j++) {
				int error = colorDelta(cell[i], cell[j]);
				if (error > bestError) {
					bestError = error;
					farthest0 = cell[i];
					farthest1 = cell[j];
				}
			}
		}
		fourColorLight[1] = farthest0;
		fourColorDark[1] = farthest1;

		// average pixels closest to each selected color
		uint16_t accumRD = 0, accumGD = 0, accumBD = 0, accumRL = 0, accumGL = 0, accumBL = 0;
		uint8_t lightCount = 0, darkCount = 0;
		for (i = 0; i < 16; i++) {
			uint32_t errorDark = colorDelta(cell[i], farthest1), errorLight = colorDelta(cell[i], farthest0);
			pixelToRGB(cell[i], &r, &g, &b);
			if (errorDark < errorLight) {
				accumRD += r, accumGD += g, accumBD += b;
				darkCount++;
			}
			else {
				accumRL += r, accumGL += g, accumBL += b;
				lightCount++;
			}
		}
		if (lightCount)
			farthest0 = rgbToPixel((accumRL + (lightCount >> 1)) / lightCount, (accumGL + (lightCount >> 1)) / lightCount, (accumBL + (lightCount >> 1)) / lightCount);
		if (darkCount)
			farthest1 = rgbToPixel((accumRD + (darkCount >> 1)) / darkCount, (accumGD + (darkCount >> 1)) / darkCount, (accumBD + (darkCount >> 1)) / darkCount);
		fourColorLight[2] = farthest0;
		fourColorDark[2] = farthest1;

		for (pass = 0; pass < 3; pass++) {
			uint32_t cellError;

			// calculate intermediate colors
			pixelToRGB(fourColorLight[pass], &rl, &gl, &bl);
			pixelToRGB(fourColorDark[pass], &rd, &gd, &bd);

			fourColors[0] = fourColorLight[pass];	// 100% light
			fourColors[2] = rgbToPixel(((rl * 3) + rd + 2) >> 2, ((gl * 3) + gd + 2) >> 2, ((bl * 3) + bd + 2) >> 2);		// 75% light
			fourColors[1] = rgbToPixel(((rd * 3) + rl + 2) >> 2, ((gd * 3) + gl + 2) >> 2, ((bd * 3) + bl + 2) >> 2);		// 25% light
			fourColors[3] = fourColorDark[pass];	// 0% light

			// assign pixels to colors
			for (i = 0, fourColorBitmap[pass] = 0, cellError = 0; i < 16; i++) {
				uint8_t bestIndex = 0, j;
				uint32_t bestError = colorDelta(fourColors[0], cell[i]);
				for (j = 1; (j < 4) && bestError; j++) {
					uint32_t error = colorDelta(fourColors[j], cell[i]);
					if (error < bestError) {
						bestError = error;
						bestIndex = j;
					}
				}
				fourColorBitmap[pass] |= bestIndex << ((15 - i) << 1);
				cellError += bestError;
			}

			if (cellError <= bestPassError) {
				bestPassError = cellError;
				bestPass = pass;
			}
		}
		}

		/*
			Output a cell
		*/
//@@ check that threshold of next type is significantly better.
		if (errorOneColor <= thresholdOneColor) {
			// solid color cell

			if (previousIsSolid && (previousSolidColor == colorAverage)) {		//@@ threshold
				command = cc->output[previousSolidOffset];
				if (7 != (command & 7)) {
					cc->output[previousSolidOffset] += 1;				// 3 bits of count
					continue;
				}
			}

			command = 0 << 5;
			if (colorDelta(colorAverage, cc->prevLight) <= kThresholdColorReuse)
				command |= kReuse0Mask;
			else
			if (colorDelta(colorAverage, cc->prevDark) <= kThresholdColorReuse)
				command |= kReuse1Mask;
			else
				cc->prevLight = colorAverage;

			if (0 == (command & (kReuse0Mask | kReuse1Mask))) {
				uint8_t r = (colorAverage >> 11);
				if (colorDelta((r << 11) | (r << 6) | r, colorAverage) <= 2) {
					command = 4 << 5;
					outputByte(the, cc, command | r);
					previousIsSolid = 0;
					continue;
				}
			}

			previousSolidOffset = cc->outputOffset;
			outputByte(the, cc, command);
			if (0 == (command & (kReuse0Mask | kReuse1Mask)))
				outputPixel(the, cc, colorAverage);

			previousIsSolid = 1;
			previousSolidColor = colorAverage;
			continue;
		}

		previousIsSolid = 0;
		if (errorTwoColor <= thresholdTwoColor) {
			// two color cell
			command = 1 << 5;
			if (colorDelta(twoColorLight, cc->prevLight) <= kThresholdColorReuse)
				command |= kReuse0Mask;
			if (colorDelta(twoColorDark, cc->prevDark) <= kThresholdColorReuse)
				command |= kReuse1Mask;

			if (0 == (command & (kReuse0Mask | kReuse1Mask))) {
				uint8_t rl = (twoColorLight >> 11);
				uint8_t rd = (twoColorDark >> 11);
				if ((colorDelta((rl << 11) | (rl << 6) | rl, twoColorLight) <= 2) && (colorDelta((rd << 11) | (rd << 6) | rd, twoColorDark) <= 2)) {
					command = 5 << 5;
					outputByte(the, cc, command | rl);
					outputByte(the, cc, rd);
					outputUint16(the, cc, twoColorBitmap);
					cc->prevLight = twoColorLight;
					cc->prevDark = twoColorDark;
					continue;
				}
			}

			outputByte(the, cc, command);
			if (!(command & kReuse0Mask)) {
				outputPixel(the, cc, twoColorLight);
				cc->prevLight = twoColorLight;
			}
			if (!(command & kReuse1Mask)) {
				outputPixel(the, cc, twoColorDark);
				cc->prevDark = twoColorDark;
			}
			outputUint16(the, cc, twoColorBitmap);
			continue;
		}

		if (errorFourColor <= thresholdFourColor) {
			// four color cell
			command = 2 << 5;
			if (colorDelta(fourColorLight[bestPass], cc->prevLight) <= kThresholdColorReuse)
				command |= kReuse0Mask;
			if (colorDelta(fourColorDark[bestPass], cc->prevDark) <= kThresholdColorReuse)
				command |= kReuse1Mask;

			outputByte(the, cc, command);
			if (!(command & kReuse0Mask)) {
				outputPixel(the, cc, fourColorLight[bestPass]);
				cc->prevLight = fourColorLight[bestPass];
			}
			if (!(command & kReuse1Mask)) {
				outputPixel(the, cc, fourColorDark[bestPass]);
				cc->prevDark = fourColorDark[bestPass];
			}
			outputUint32(the, cc, fourColorBitmap[bestPass]);

			continue;
		}

		// sixteen color cell (uncompressed)
		command = 3 << 5;
		outputByte(the, cc, command);
		for (i = 0; i < 16; i++)
			outputPixel(the, cc, cell[i]);

		continue;
	}
}

// note: calculations based on 6 bits per component, though low bit of R and B is always 0.
// This is done, instead of 5 bits per component, to include low green bit while maintaining equal weight for each component.
uint32_t colorDelta(PocoPixel p0, PocoPixel p1)
{
	int16_t r = (p0 >> 11) - (p1 >> 11);
	int16_t g = ((p0 >> 5) & 0x3f) - ((p1 >> 5) & 0x3f);
	int16_t b = (p0 & 0x1f) - (p1 & 0x1f);

	if (g < 0) g = -g;
	g = (g + 1) >> 1;	// remove extra bit from green

	return (r * r) + (g * g) + (b * b);
}

void pixelToRGB(PocoPixel pixel, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = (pixel >> 11);
	*g = (pixel >> 5) & 0x3f;
	*b = pixel & 0x1f;
}

PocoPixel rgbToPixel(uint8_t r, uint8_t g, uint8_t b)
{
	return (r << 11) | (g << 5) | b;
}

