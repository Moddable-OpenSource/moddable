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


/*
	Commodetto readPNG

		no interlace
		no 16-bit channels

	To do:

		extract and return palette

	N.B.
	
		Modeled on FskPngDecode.c (Apache License, Marvell Semiconductor)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

#include "commodettoPocoBlit.h"

#if defined(__GNUC__) /* GCC */
	#pragma GCC diagnostic ignored "-Wmultichar"
#endif

// 4 channels @ 8 bits per pixel = 4 bytes
#define kScanLineSlop (4)

typedef struct {
	int				byteOffset;
	xsSlot			data;
	int				byteLength;
	int				idatBytes;

	int				width;
	int				height;
	int				row;
	mz_ulong		scanLineByteCount;
	uint8_t			channelCount;
	uint8_t			bitDepth;
	uint8_t			filterBytesPerPixel;

	uint8_t			zlibInited;

	xsSlot			scanLineSlot;

	unsigned char	*scanBuffers;
	unsigned char	*scanLine;
	unsigned char	*prevScanLine;

	z_stream		zlib;
} PNGRecord, *PNG;

static void *zAlloc(void *opaque, size_t items, size_t size)
{
	return malloc(items * size);
}

static void zFree(void *opaque, void *address)
{
	if (address)
		free(address);
}

void pngClose(PNG png)
{
	if (png->zlibInited) {
		inflateEnd(&png->zlib);
		png->zlibInited = 0;
	}
	if (png->scanBuffers) {
		free(png->scanBuffers);
		png->scanBuffers = NULL;
	}
}

void xs_PNG_destructor(void *data)
{
	PNG png = data;
	if (png) {
		pngClose(png);
		free(png);
	}
}

static void scanDestructor(void *data)
{
	// managed by PNG (imperfect, but...)
}

static void emptyFilter(uint8_t *pix, uint8_t* prev, uint32_t width, uint8_t bpp)
{
}

static void subFilter(uint8_t *pix, uint8_t* prev, uint32_t width, uint8_t bpp)
{
	prev = pix - bpp;
	while (width--)
		*pix++ += *prev++;
}

static void upFilter(uint8_t *pix, uint8_t* prev, uint32_t width, uint8_t bpp)
{
	while (width--)
		*pix++ += *prev++;
}

static void averageFilter(uint8_t *pix, uint8_t *prev, uint32_t width, uint8_t bpp)
{
	uint8_t *prior = pix - bpp;
	while (width--)
		*pix++ += (*prev++ + *prior++) >> 1;
}

static void paethFilter(uint8_t *pix, uint8_t *prev, uint32_t width, uint8_t bpp)
{
	unsigned char *prior = pix - bpp;
	unsigned char *prevPrior = prev - bpp;

	while (width--) {
		int16_t a = *prior++;
		int16_t b = *prev++;
		int16_t c = *prevPrior++;
		int16_t p = a + b - c;
		int16_t pa = p - a;
		int16_t pb = p - b;
		int16_t pc = p - c;
		if (pa < 0) pa = -pa;
		if (pb < 0) pb = -pb;
		if (pc < 0) pc = -pc;
		if ((pa <= pb) && (pa <= pc))
			p = a;
		else if (pb <= pc)
			p = b;
		else
			p = c;

		*pix++ += p;
	}
}

typedef void (*PNG_Filter)(uint8_t *pix, uint8_t* prev, uint32_t width, uint8_t bpp);

const PNG_Filter gFilters[5] ICACHE_RODATA_ATTR = {
	emptyFilter,
	subFilter,
	upFilter,
	averageFilter,
	paethFilter
};

void xs_PNG_constructor(xsMachine *the)
{
	PNG png;
	const unsigned char *pngBytes, *pngBytesInitial;
	int tagCount = 0;
	int8_t colorType, compressionMethod, filterMethod, interlaceMethod, bitsPerPixel;
	xsUnsignedValue dataSize;

	png = calloc(1, sizeof(PNGRecord));
	if (!png)
		xsErrorPrintf("no memory for PNG");
	xsmcSetHostData(xsThis, png);

	xsmcVars(2);

	png->data = xsArg(0);
	xsmcSet(xsThis, xsID_buffer, png->data);

	xsmcGetBufferReadable(xsArg(0), (void **)&pngBytesInitial, &dataSize);
	pngBytes = pngBytesInitial;
	png->byteLength = dataSize;

	if ((0x89 != pngBytes[0]) || (0x50 != pngBytes[1]) || (0x4e != pngBytes[2]) || (0x47 != pngBytes[3]) ||
		(0x0d != pngBytes[4]) || (0x0a != pngBytes[5]) || (0x1a != pngBytes[6]) || (0x0a != pngBytes[7]))
		xsErrorPrintf("invalid PNG signature");

	pngBytes += 8; png->byteLength -= 8;

	while (png->byteLength >= 12) {
		int tagLen;
		uint32_t tag;

		tagLen = (pngBytes[0] << 24) | (pngBytes[1] << 16) | (pngBytes[2] << 8) | pngBytes[3];
		tag = (pngBytes[4] << 24) | (pngBytes[5] << 16) | (pngBytes[6] << 8) | pngBytes[7];

		pngBytes += 8;
		png->byteLength -= 8;

		if (tagLen < 0)
			xsErrorPrintf("corrupt tag length");

		if ((png->byteLength - 4) <  tagLen)
			xsErrorPrintf("incomplete PNG tag");

		if (1 == ++tagCount) {
			if ('IHDR' != tag)
				xsErrorPrintf("first PNG tag must be IHDR");
		}

		switch (tag) {
			case 'IHDR':
				png->width = (pngBytes[0] << 24) | (pngBytes[1] << 16) | (pngBytes[2] << 8) | pngBytes[3];
				png->height = (pngBytes[4] << 24) | (pngBytes[5] << 16) | (pngBytes[6] << 8) | pngBytes[7];

				png->bitDepth = pngBytes[8];
				colorType = pngBytes[9];
				compressionMethod = pngBytes[10];
				filterMethod = pngBytes[11];
				interlaceMethod = pngBytes[12];

				// check for undefined values
				if ((compressionMethod != 0) || (filterMethod != 0) || (interlaceMethod != 0))
					xsErrorPrintf("invalid compression, filter, or interlace method");

				// sort ouf the bitmap format
				if (6 == colorType)
					png->channelCount = 4;
				else if (2 == colorType)
					png->channelCount = 3;
				else if (3 == colorType)
					png->channelCount = 1;
				else if (0 == colorType)
					png->channelCount = 1;
				else if (4 == colorType)
					png->channelCount = 2;
				bitsPerPixel = png->channelCount * png->bitDepth;
				png->filterBytesPerPixel = (bitsPerPixel + 7) >> 3;

				//@@ check for supported combinations

				png->scanLineByteCount = (png->width * bitsPerPixel + 7) >> 3;
				png->scanBuffers = malloc((png->scanLineByteCount + kScanLineSlop) * 2);
				if (!png->scanBuffers)
					xsErrorPrintf("no memory for scan line buffers");

				png->scanLine = png->scanBuffers;
				png->prevScanLine = png->scanBuffers + (png->scanLineByteCount + kScanLineSlop);
				c_memset(png->prevScanLine, 0, png->scanLineByteCount + kScanLineSlop);
#if 4 == kScanLineSlop
				*(uint32_t *)png->scanLine = 0;
				*(uint32_t *)png->prevScanLine = 0;
#else
				@@ need to implement clear of kScanLineSlop
#endif

				png->zlib.zalloc = zAlloc;
				png->zlib.zfree = zFree;
				if (Z_OK != inflateInit2(&png->zlib, 15))		//@@ FskPNGDecode uses -15. which fails here. 15 works. why?
					xsErrorPrintf("can't init zlib");
				png->zlibInited = 1;
				break;

			case 'PLTE': {
				int colors = tagLen / 3;
				const unsigned char *src = pngBytes;
				unsigned char *dst;

				xsmcSetArrayBuffer(xsVar(0), NULL, colors * 4);
				xsmcSet(xsThis, xsID_palette, xsVar(0));
				dst = xsmcToArrayBuffer(xsVar(0));

				while (colors--) {
					*dst++ = *src++;		// r
					*dst++ = *src++;		// g
					*dst++ = *src++;		// b
					*dst++ = 255;			// a
				}
				}
				break;

			case 'tRNS':
				xsmcGet(xsVar(0), xsThis, xsID_palette);
				if (xsmcTest(xsVar(0))) {
					int colors = tagLen;
					const unsigned char *src = pngBytes;
					unsigned char *dst = (unsigned char *)xsmcToArrayBuffer(xsVar(0)) + 3;

					while (colors--) {
						*dst = *src++;
						dst += 4;
					}
				}
				break;

			case 'IEND':
				xsErrorPrintf("no IDAT");
				break;

			case 'IDAT':
				png->idatBytes = tagLen;
				png->byteOffset = pngBytes - pngBytesInitial;

				xsVar(1) = xsNewHostObject(scanDestructor);
				xsmcSetHostBuffer(xsVar(1), NULL, png->scanLineByteCount);
				png->scanLineSlot = xsVar(1);

				xsVar(1) = xsNew1(xsGlobal, xsID_Uint8Array, xsVar(1));
				xsmcSet(xsThis, xsID_data, xsVar(1));
				return;
		}

		pngBytes += tagLen + 4; png->byteLength -= tagLen + 4;
	}
}

void xs_PNG_read(xsMachine *the)
{
	PNG png = xsmcGetHostData(xsThis);
	const unsigned char *pngBytes, *pngBytesInitial;
	unsigned char *swap;
	int result;
	uint8_t filter;
	xsUnsignedValue size;

	if (png->row++ >= png->height)
		return;

	// refresh source data pointer
	xsmcGetBufferReadable(png->data, (void **)&pngBytesInitial, &size);
	pngBytes = pngBytesInitial + png->byteOffset;

	// decompress another scan line
	png->zlib.next_out	= png->scanLine + kScanLineSlop - 1;
	png->zlib.avail_out	= png->scanLineByteCount + 1;
	png->zlib.total_out	= 0;

	do {
		png->zlib.next_in = pngBytes;
		png->zlib.avail_in = png->idatBytes;
		png->zlib.total_in = 0;

		result = inflate(&png->zlib, Z_PARTIAL_FLUSH);
		if ((Z_OK != result) && (Z_STREAM_END != result)) {
			if (Z_DATA_ERROR == result)
				xsErrorPrintf("bad zlib data");
		}

		pngBytes += png->zlib.total_in;
		png->idatBytes -= png->zlib.total_in;
		if (png->idatBytes <= 0) {
			// look for next IDAT (required to be consecutive)
			uint32_t tag;

			tag = (pngBytes[8] << 24) | (pngBytes[9] << 16) | (pngBytes[10] << 8) | pngBytes[11];
			if ('IDAT' == tag) {
				png->idatBytes = (pngBytes[4] << 24) | (pngBytes[5] << 16) | (pngBytes[6] << 8) | pngBytes[7];
				pngBytes += 12;
			}
		}
	} while (png->zlib.total_out != (png->scanLineByteCount + 1));

	// filter scan line
	filter = png->scanLine[kScanLineSlop - 1];
	if (filter > 4)
		xsErrorPrintf("invalid scan line filter");
	png->scanLine[kScanLineSlop - 1] = 0;
	gFilters[filter](png->scanLine + kScanLineSlop, png->prevScanLine + kScanLineSlop, png->scanLineByteCount, png->filterBytesPerPixel);

	// return a scan line of data
	xsmcGet(xsResult, xsThis, xsID_data);
	xsmcSetHostBuffer(png->scanLineSlot, png->scanLine + kScanLineSlop, png->scanLineByteCount);

	// swap previous and current scan line buffers
	swap = png->prevScanLine;
	png->prevScanLine = png->scanLine;
	png->scanLine = swap;

	// update source data offset
	png->byteOffset = pngBytes - pngBytesInitial;
}

void xs_PNG_get_width(xsMachine *the)
{
	PNG png = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, png->width);
}

void xs_PNG_get_height(xsMachine *the)
{
	PNG png = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, png->height);
}

void xs_PNG_get_channels(xsMachine *the)
{
	PNG png = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, png->channelCount);
}

void xs_PNG_get_depth(xsMachine *the)
{
	PNG png = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, png->bitDepth);
}
