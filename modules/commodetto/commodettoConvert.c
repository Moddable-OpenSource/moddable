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


#include "commodettoConvert.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#ifdef __ets__
	#include "xsesp.h"
#else
	#define ICACHE_XS6RO_ATTR
	#define ICACHE_XS6RO2_ATTR
	#define ICACHE_RODATA_ATTR
#endif

/*
	XS binding
*/

typedef struct {
	CommodettoBitmapFormat	srcPixelFormat;
	CommodettoBitmapFormat	dstPixelFormat;
	uint8_t					srcPixelDepth;
	uint8_t					dstPixelDepth;

	void					*clut;

	CommodettoConverter		converter;
} xsConvertRecord,  *xsConvert;

void xs_Convert_destructor(void *data)
{
	if (data) {
		xsConvert c = data;
		if (c->clut)
			free(c->clut);
	}
}

void xs_Convert(xsMachine *the)
{
	xsConvert c = xsmcSetHostChunk(xsThis, NULL, sizeof(xsConvertRecord));

	c->srcPixelFormat = (CommodettoBitmapFormat)xsmcToInteger(xsArg(0));
	c->dstPixelFormat = (CommodettoBitmapFormat)xsmcToInteger(xsArg(1));

	c->srcPixelDepth = CommodettoBitmapGetDepth(c->srcPixelFormat);
	c->dstPixelDepth = CommodettoBitmapGetDepth(c->dstPixelFormat);

	c->converter = CommodettoPixelsConverterGet(c->srcPixelFormat, c->dstPixelFormat);
	if (NULL == c->converter)
		xsErrorPrintf("conversion not supported");

	if (kCommodettoBitmapCLUT16 == c->dstPixelFormat) {
		void *clut;
		uint32_t clutBytes;

		if (!xsmcTest(xsArg(2)))
			xsErrorPrintf("clut required");

		if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
			clut = xsmcToArrayBuffer(xsArg(2));
			clutBytes = xsGetArrayBufferLength(xsArg(2));
		}
		else {
			xsmcVars(1);
			clut = xsmcGetHostData(xsArg(2));
			xsmcGet(xsVar(0), xsArg(2), xsID_byteLength);
			clutBytes = xsmcToInteger(xsVar(0));
		}

		c->clut = malloc(clutBytes);
		if (NULL == c->clut)
			xsErrorPrintf("not enough memory to clone clut");
		memcpy(c->clut, clut, clutBytes);
	}
}

void xs_convert_process(xsMachine *the)
{
	xsConvert c = xsmcGetHostChunk(xsThis);
	uint8_t *src, *dst;
	int srcLength, dstLength, pixelCount;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		src = xsmcToArrayBuffer(xsArg(0));
		srcLength = xsGetArrayBufferLength(xsArg(0));
	}
	else {
		xsmcVars(1);
		src = xsmcGetHostData(xsArg(0));
		xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
		srcLength = xsmcToInteger(xsVar(0));
	}
	dst = xsmcToArrayBuffer(xsArg(1));
	dstLength = xsGetArrayBufferLength(xsArg(1));

	pixelCount = (srcLength << 3) / c->srcPixelDepth;

	if (((dstLength << 3) / c->dstPixelDepth) < pixelCount)
		xsErrorPrintf("dst buffer too small");

	(c->converter)((uint32_t)pixelCount, src, dst, c->clut);
}

/*
	pixel converter implementation
*/

static void ccCopy4(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccCopy8(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccCopy16(uint32_t pixelCount, void *src, void *dst, void *clut);

static void ccGray16toGray256(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccGray16toRGB332(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccGray16toRGB565LE(uint32_t pixelCount, void *src, void *dst, void *clut);

static void ccGray256toMonochrome(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccGray256toGray16(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccGray256toRGB332(uint32_t pixelCount, void *src, void *dst, void *clut);
static void ccGray256toRGB565LE(uint32_t pixelCount, void *src, void *dst, void *clut);

static void ccRGB565LEtoGray256(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut);

static void cc24RGBtoGray16(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc24RGBtoGray256(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc24RGBtoRGB332(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc24RGBtoRGB565LE(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc24RGBtoCLUT16(uint32_t pixelCount, void *src, void *dst, void *clut);

static void cc32RGBAtoGray16(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc32RGBAtoGray256(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc32RGBAtoRGB332(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc32RGBAtoRGB565LE(uint32_t pixelCount, void *src, void *dst, void *clut);
static void cc32RGBAtoCLUT16(uint32_t pixelCount, void *src, void *dst, void *clut);

static const CommodettoConverter gFromGray16[] ICACHE_XS6RO_ATTR = {
	NULL,					// toMonochrome
	ccCopy4,				// toGray16
	ccGray16toGray256,		// toGray256
	ccGray16toRGB332,		// toRGB332
	ccGray16toRGB565LE,		// toRGB565LE
	NULL,					// toRGB565BE
	NULL,					// to24RGB
	NULL,					// to32RGBA
	NULL					// toCLUT16
};

static const CommodettoConverter gFromGray256[] ICACHE_XS6RO_ATTR = {
	ccGray256toMonochrome,	// toMonochrome
	ccGray256toGray16,		// toGray16
	ccCopy8,				// toGray256
	ccGray256toRGB332,		// toRGB332
	ccGray256toRGB565LE,	// toRGB565LE
	NULL,					// toRGB565BE
	NULL,					// to24RGB
	NULL,					// to32RGBA
	NULL					// toCLUT16
};

static const CommodettoConverter gFromRGB565LE[] ICACHE_XS6RO_ATTR = {
	NULL,					// toMonochrome
	NULL,					// toGray16
	ccRGB565LEtoGray256,	// toGray256
	NULL,					// toRGB332
	ccCopy16,				// toRGB565LE
	NULL,					// toRGB565BE
	NULL,					// to24RGB
	NULL,					// to32RGBA
	NULL					// toCLUT16
};

static const CommodettoConverter gFrom24RGB[] ICACHE_XS6RO_ATTR = {
	NULL,					// toMonochrome
	cc24RGBtoGray16,		// toGray16
	cc24RGBtoGray256,		// toGray256
	cc24RGBtoRGB332,		// toRGB332
	cc24RGBtoRGB565LE,		// toRGB565LE
	NULL,					// toRGB565BE
	NULL,					// to24RGB
	NULL,					// to32RGBA
	cc24RGBtoCLUT16			// toCLUT16
};

static const CommodettoConverter gFrom32RGBA[] ICACHE_XS6RO2_ATTR = {		// pre-multiplied alpha
	NULL,					// toMonochrome
	cc32RGBAtoGray16,		// toGray16
	cc32RGBAtoGray256,		// toGray256
	cc32RGBAtoRGB332,		// toRGB332
	cc32RGBAtoRGB565LE,		// toRGB565LE
	NULL,					// toRGB565BE
	NULL,					// to24RGB
	NULL,					// to32RGBA
	cc32RGBAtoCLUT16		// toCLUT16
};

static const CommodettoConverter *gFromConverters[] ICACHE_RODATA_ATTR = {
	NULL,				// fromMonochrome
	gFromGray16,		// fromGray16
	gFromGray256,		// fromGray256
	NULL,				// fromRGB332
	gFromRGB565LE,		// fromRGB565LE
	NULL,				// fromRGB565BE
	gFrom24RGB,			// from24RGB
	gFrom32RGBA			// from32RGBA
};

#define toGray(r, g, b) (((r << 1) + r + (g << 2) + b) >> 3)

uint8_t CommodettoPixelsConvert(uint32_t pixelCount,
								void *srcPixels, CommodettoBitmapFormat srcFormat,
								void *dstPixels, CommodettoBitmapFormat dstFormat)
{
	if ((srcFormat < kCommodettoBitmapMonochrome) || (dstFormat < kCommodettoBitmapMonochrome))
		return 0;

	if ((srcFormat > kCommodettoBitmap32RGBA) || (dstFormat > kCommodettoBitmap32RGBA))
		return 0;

	(gFromConverters[srcFormat - kCommodettoBitmapMonochrome])[dstFormat - kCommodettoBitmapMonochrome](pixelCount, srcPixels, dstPixels, NULL);

	return 1;
}

CommodettoConverter CommodettoPixelsConverterGet(CommodettoBitmapFormat srcFormat, CommodettoBitmapFormat dstFormat)
{
	if ((srcFormat < kCommodettoBitmapMonochrome) || (dstFormat < kCommodettoBitmapMonochrome))
		return 0;

	if ((srcFormat > kCommodettoBitmap32RGBA) || (dstFormat > kCommodettoBitmapCLUT16))
		return 0;

	return (gFromConverters[srcFormat - kCommodettoBitmapMonochrome])[dstFormat - kCommodettoBitmapMonochrome];
}

void ccCopy4(uint32_t pixelCount, void *src, void *dst, void *clut)
{
	memcpy(dst, src, (pixelCount + 1) >> 1);
}

void ccCopy8(uint32_t pixelCount, void *src, void *dst, void *clut)
{
	memcpy(dst, src, pixelCount);
}

void ccCopy16(uint32_t pixelCount, void *src, void *dst, void *clut)
{
	memcpy(dst, src, pixelCount << 1);
}

void ccGray256toMonochrome(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;
	uint8_t mono = 0;
	uint8_t mask = 0x80;

	while (pixelCount--) {
		if (*src++ >= 128)
			mono |= mask;

		mask >>= 1;
		if (0 == mask) {
			*dst++ = mono;
			mono = 0;
			mask = 0x80;
		}
	}

	if (0x80 != mask)
		*dst++ = mono;
}

void ccGray256toGray16(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t gray256_0 = *src++;
		uint8_t gray256_1 = *src++;
		*dst++ = (gray256_0 & 0xf0) | (gray256_1 >> 4);
		pixelCount -= 2;
	}
	if (pixelCount)
		*dst++ = *src & 0xf0;
}

void ccGray256toRGB332(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		uint8_t gray = *src++ >> 5;
		*dst++ = (gray << 5) | (gray << 2) | (gray >> 1);
	}
}

void ccGray256toRGB565LE(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint16_t *dst = dstPixels;

	while (pixelCount--) {
		uint8_t gray256 = *src++;
		*dst++ = ((gray256 >> 3) << 11) | ((gray256 >> 2) << 5) | (gray256 >> 3);
	}
}

void ccRGB565LEtoGray256(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint16_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		uint16_t srcPixel = *src++;
		uint8_t r = srcPixel >> 11;
		uint8_t g = (srcPixel >> 5) & 0x3F;
		uint8_t b = (srcPixel & 0x1F);

		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);

		*dst++ = toGray(r, g, b);
	}
}

void ccGray16toGray256(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t twoPixels = *src++;
		uint8_t gray16_0 = twoPixels >> 4;
		uint8_t gray16_1 = twoPixels & 0x0f;
		*dst++ = (gray16_0 << 4) | gray16_0;
		*dst++ = (gray16_1 << 4) | gray16_1;
		pixelCount -= 2;
	}

	if (pixelCount) {
		uint8_t gray16_0 = *src >> 4;
		*dst++ = (gray16_0 << 4) | gray16_0;
	}
}

void ccGray16toRGB332(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t twoPixels = *src++;
		uint8_t gray16_0 = twoPixels >> 5;
		uint8_t gray16_1 = (twoPixels & 0x0f) >> 1;
		*dst++ = (gray16_0 << 5) | (gray16_0 << 2) | (gray16_0 >> 1);
		*dst++ = (gray16_1 << 5) | (gray16_1 << 2) | (gray16_1 >> 1);
		pixelCount -= 2;
	}

	if (pixelCount) {
		uint8_t gray16_0 = *src >> 5;
		*dst++ = (gray16_0 << 5) | (gray16_0 << 2) | (gray16_0 >> 1);
	}
}

void ccGray16toRGB565LE(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint16_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t twoPixels = *src++;
		uint8_t gray16_0 = ((twoPixels & 0xf0) >> 3) | (twoPixels >> 7);
		uint8_t gray16_1 = ((twoPixels & 0x0f) << 1) | ((twoPixels & 0x0f) >> 3);

		*dst++ = (gray16_0 << 11) | (gray16_0 << 6) | gray16_0;
		*dst++ = (gray16_1 << 11) | (gray16_1 << 6) | gray16_1;
		pixelCount -= 2;
	}

	if (pixelCount) {
		uint8_t twoPixels = *src++;
		uint8_t gray16_0 = ((twoPixels & 0xf0) >> 3) | (twoPixels >> 7);
		*dst++ = (gray16_0 << 11) | (gray16_0 << 6) | gray16_0;
	}
}

void cc24RGBtoGray16(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t gray16_0 = toGray(src[0], src[1], src[2]) >> 4;
		uint8_t gray16_1 = toGray(src[3], src[4], src[5]) >> 4;
		src += 6;
		*dst++ = (gray16_0 << 4) | gray16_1;
		pixelCount -= 2;
	}

	if (pixelCount) {
		uint8_t gray16_0 = toGray(src[0], src[1], src[2]) >> 4;
		*dst++ = gray16_0 << 4;
	}
}

void cc24RGBtoGray256(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		uint8_t r = src[0];
		uint8_t g = src[1];
		uint8_t b = src[2];
		src += 3;

		*dst++ = toGray(r, g, b);
	}
}

void cc24RGBtoRGB332(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		*dst++ = (src[0] & 0xE0) | ((src[1] >> 5) << 2) | (src[2] >> 6);
		src += 3;
	}
}

void cc24RGBtoRGB565LE(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint16_t *dst = dstPixels;

	while (pixelCount--) {
		*dst++ = ((src[0] >> 3) << 11) | ((src[1] >> 2) << 5) | (src[2] >> 3);
		src += 3;
	}
}

void cc32RGBAtoGray16(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount >= 2) {
		uint8_t gray16_0 = toGray(src[0], src[1], src[2]) >> 4;
		uint8_t gray16_1 = toGray(src[4], src[5], src[6]) >> 4;

		src += 8;
		*dst++ = (gray16_0 << 4) | gray16_1;
		pixelCount -= 2;
	}

	if (pixelCount) {
		uint8_t gray16_0 = toGray(src[0], src[1], src[2]) >> 4;
		*dst++ = gray16_0 << 4;
	}
}

void cc32RGBAtoGray256(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		uint8_t r = src[0];
		uint8_t g = src[1];
		uint8_t b = src[2];
		src += 4;

		*dst++ = toGray(r, g, b);
	}
}

void cc32RGBAtoRGB332(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;

	while (pixelCount--) {
		*dst++ = (src[0] & 0xE0) | ((src[1] >> 5) << 2) | (src[2] >> 6);
		src += 4;
	}
}

void cc32RGBAtoRGB565LE(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint16_t *dst = dstPixels;

	while (pixelCount--) {
		*dst++ = ((src[0] >> 3) << 11) | ((src[1] >> 2) << 5) | (src[2] >> 3);
		src += 4;
	}
}

void cc24RGBtoCLUT16(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;
	uint8_t *inverse = (uint8_t *)clut + (sizeof(uint16_t) * 16);

	while (pixelCount >= 2) {
		uint16_t colorA, colorB;

		colorA = ((src[0] & 0xF0) << 4) | (src[1] & 0xF0) | (src[2] >> 4);
		colorB = ((src[3] & 0xF0) << 4) | (src[4] & 0xF0) | (src[5] >> 4);
		*dst = (inverse[colorA] << 4) | inverse[colorB];

		src += 6;
		dst += 1;
		pixelCount -= 2;
	}

	if (pixelCount)
		*dst++ = inverse[((src[0] & 0xF0) << 4) | (src[1] & 0xF0) | (src[2] >> 4)] << 4;
}

void cc32RGBAtoCLUT16(uint32_t pixelCount, void *srcPixels, void *dstPixels, void *clut)
{
	uint8_t *src = srcPixels;
	uint8_t *dst = dstPixels;
	uint8_t *inverse = (uint8_t *)clut + (sizeof(uint16_t) * 16);

	while (pixelCount >= 2) {
		uint16_t colorA, colorB;

		colorA = ((src[0] & 0xF0) << 4) | (src[1] & 0xF0) | (src[2] >> 4);
		colorB = ((src[4] & 0xF0) << 4) | (src[5] & 0xF0) | (src[6] >> 4);
		*dst = (inverse[colorA] << 4) | inverse[colorB];

		src += 8;
		dst += 1;
		pixelCount -= 2;
	}

	if (pixelCount)
		*dst++ = inverse[((src[0] & 0xF0) << 4) | (src[1] & 0xF0) | (src[2] >> 4)] << 4;
}
