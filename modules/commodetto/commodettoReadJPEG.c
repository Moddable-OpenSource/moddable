/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
	Commodetto readJPEG

		wraps picojpeg
		
	To do:

		 PJPG_YH1V2

		sliding CommodettoBitmap (e.g. pixels in chunk rather than malloc)

		JPEG record as chunk
*/

#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "xsPlatform.h"

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

#include "commodettoPocoBlit.h"

#include "picojpeg.h"
#include "picojpeg.c"

typedef void (*convertto)(void *jpeg, CommodettoBitmap cb, PocoPixel *pixels);

typedef struct {
	int					position;
	xsMachine			*the;
	uint8_t				*pixels;
	convertto			convert;
	int					totalBytesAvailable;
	uint16_t			blocksRemaining;
	int					bytesInBuffer;
	CommodettoBitmapFormat pixelFormat;
	uint8_t				pixelSize;
	uint8_t				bufferIndex;
	uint8_t				endOfData;

	unsigned char		blockWidth;
	unsigned char		blockYMax;			 // info.m_MCUSPerCol - 1
	unsigned char		blockX;
	unsigned char		blockY;

	unsigned char		mcuWidth;
	unsigned char		mcuHeight;
	unsigned char		mcuWidthRight;
	unsigned char		mcuHeightBottom;

	pjpeg_scan_type_t	scanType;

	unsigned char		*r;
	unsigned char		*g;
	unsigned char		*b;
} JPEGRecord, *JPEG;

static uint8_t tryInitialize(xsMachine *the, JPEG jpeg);
static unsigned char needBytes(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
static void activateBuffer(JPEG jpeg);

#if (16 == kPocoPixelSize) || (8 == kPocoPixelSize)
	static void convertto_16and8(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels);
#endif
#if 4 == kPocoPixelSize
	static void convertto_4(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels);
#endif
static void convertto_24(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels);

void xs_JPEG_destructor(void *data)
{
	if (data)
		c_free(data);
}

static void pixelsDestructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_JPEG_constructor(xsMachine *the)
{
	JPEG jpeg;
	int argc = xsmcArgc;

	xsmcVars(2);

	jpeg = c_calloc(sizeof(JPEGRecord), 1);
	if (!jpeg)
		xsErrorPrintf("jpeg out of memory");

	xsmcSetHostData(xsThis, jpeg);

	jpeg->the = the;

	jpeg->pixelFormat = kCommodettoBitmapFormat;
	if ((argc > 1) && (xsUndefinedType != xsmcTypeOf(xsArg(1))) && xsmcHas(xsArg(1), xsID_pixelFormat)) {
		xsmcGet(xsVar(0), xsArg(1), xsID_pixelFormat);
		jpeg->pixelFormat = xsmcToInteger(xsVar(0));
	}

#if (16 == kPocoPixelSize) || (8 == kPocoPixelSize)
	jpeg->convert = (convertto)convertto_16and8;
	jpeg->pixelSize = kPocoPixelSize;
#endif
#if 4 == kPocoPixelSize
	jpeg->convert = (convertto)convertto_4;
	jpeg->pixelSize = 4;
#endif
	if (kCommodettoBitmap24RGB == jpeg->pixelFormat) {
		jpeg->convert = (convertto)convertto_24;
		jpeg->pixelSize = 24;
	}
	else if (kCommodettoBitmapFormat != jpeg->pixelFormat)
		xsErrorPrintf("unsupported pixel format");

	xsVar(0) = xsNewArray(0);
	xsmcSet(xsThis, xsID_buffers, xsVar(0));

	if ((0 == argc) || !xsmcTest(xsArg(0)))
		return;

	jpeg->endOfData = true;
	xsCall1(xsThis, xsID_push, xsArg(0));
	if (NULL == jpeg->r)
		xsErrorPrintf("jpeg init failed");
}

void xs_JPEG_push(xsMachine *the)
{
	JPEG jpeg = xsmcGetHostData(xsThis);
	void *src;
	xsUnsignedValue srcBytes;

	if (!xsmcArgc) {
		jpeg->endOfData = true;
		return;
	}

	xsmcVars(2);

	xsmcGet(xsResult, xsThis, xsID_buffers);
	xsCall1(xsResult, xsID_push, xsArg(0));

	if (0 == jpeg->totalBytesAvailable)
		activateBuffer(jpeg);

	xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcBytes);
	jpeg->totalBytesAvailable += srcBytes;

	if (NULL == jpeg->r)
		tryInitialize(the, jpeg);
}

void xs_JPEG_get_ready(xsMachine *the)
{
	JPEG jpeg = xsmcGetHostData(xsThis);
	xsmcSetBoolean(xsResult, (NULL != jpeg->r) && jpeg->blocksRemaining && (jpeg->endOfData || (jpeg->totalBytesAvailable >= 256)));
}

void xs_JPEG_read(xsMachine *the)
{
	JPEG jpeg = xsmcGetHostData(xsThis);
	unsigned char result;
	CommodettoBitmap cb;

	xsmcVars(2);

	result = pjpeg_decode_mcu();
	if (0 != result) {
		if (PJPG_NO_MORE_BLOCKS == result)
			return;
		xsErrorPrintf("jpeg read failed");
	}

	xsmcGet(xsVar(1), xsThis, xsID_bitmap);
	xsmcSetInteger(xsVar(0), jpeg->blockX * jpeg->mcuWidth);
	xsmcSet(xsVar(1), xsID_x, xsVar(0));
	xsmcSetInteger(xsVar(0), jpeg->blockY * jpeg->mcuHeight);
	xsmcSet(xsVar(1), xsID_y, xsVar(0));

	cb = xsmcGetHostChunk(xsVar(1));
	if (jpeg->blockY == jpeg->blockYMax)
		cb->h = jpeg->mcuHeightBottom;
	else
		cb->h = jpeg->mcuHeight;

	if ((jpeg->blockX + 1) == jpeg->blockWidth) {
		cb->w = jpeg->mcuWidthRight;

		jpeg->blockX = 0;
		jpeg->blockY += 1;
	}
	else {
		cb->w = jpeg->mcuWidth;

		jpeg->blockX += 1;
	}

	(jpeg->convert)(jpeg, cb, (void *)jpeg->pixels);

	xsResult = xsVar(1);

	jpeg->blocksRemaining -= 1;
}

#if (kPocoPixelSize == 16) || (kPocoPixelSize == 8)

#if kCommodettoBitmapGray256 == kPocoPixelFormat
	#define makePixel(r, g, b) PocoMakePixelGray256(r, g, b)
#elif kCommodettoBitmapRGB332 == kPocoPixelFormat
	#define makePixel(r, g, b) PocoMakePixelRGB332(r, g, b)
#elif kCommodettoBitmapRGB565LE == kPocoPixelFormat
	#define makePixel(r, g, b) PocoMakePixelRGB565LE(r, g, b)
#else
	#error
#endif

void convertto_16and8(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels)
{
	int i;
	int outWidth = cb->w;
	int outHeight = cb->h;

	if (PJPG_YH1V1 == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			int pixelCount;
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++)
				*pixels++ =	makePixel(jpeg->r[i], jpeg->g[i], jpeg->b[i]);
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++)
					*pixels++ = makePixel(jpeg->r[i], jpeg->g[i], jpeg->b[i]);
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else if (PJPG_YH2V1 == jpeg->scanType) {
		unsigned char *r, *g, *b;
		uint8_t jMaxLeft, jMaxRight;
		int j;

		if (jpeg->mcuWidth == outWidth) {
			jMaxLeft = 8;
			jMaxRight = 72;
		}
		else if (outWidth <= 8) {
			jMaxLeft = outWidth;
			jMaxRight = 0;
		} else {
			jMaxLeft = 8;
			jMaxRight = 56 + outWidth;
		}

		r = jpeg->r, g = jpeg->g, b = jpeg->b;
		for (i = (outHeight >= 8) ? 8 : outHeight; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
			for (j = 64; j < jMaxRight; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
		}
	}
	else if (PJPG_YH2V2 == jpeg->scanType) {
		unsigned char *r, *g, *b;
		uint8_t jMaxLeft, jMaxRight;
		int j;

		if (jpeg->mcuWidth == outWidth) {
			jMaxLeft = 8;
			jMaxRight = 72;
		}
		else if (outWidth <= 8) {
			jMaxLeft = outWidth;
			jMaxRight = 0;
		} else {
			jMaxLeft = 8;
			jMaxRight = 56 + outWidth;
		}

		r = jpeg->r, g = jpeg->g, b = jpeg->b;
		for (i = (outHeight >= 8) ? 8 : outHeight; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
			for (j = 64; j < jMaxRight; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
		}

		r = jpeg->r + 128, g = jpeg->g + 128, b = jpeg->b + 128;
		for (i = (outHeight > 8) ? outHeight - 8 : 0; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
			for (j = 64; j < jMaxRight; j++)
				*pixels++ =	makePixel(r[j], g[j], b[j]);
		}
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			int pixelCount;
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++) {
				PocoPixel gray = ~jpeg->r[i];
				*pixels++ = makePixel(gray, gray, gray);
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++) {
					PocoPixel gray = ~jpeg->r[i];
					*pixels++ = makePixel(gray, gray, gray);
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else
		modLog("unimplemented scan type");
}
#endif

#if kPocoPixelSize == 4

#if kCommodettoBitmapCLUT16 == kPocoPixelFormat
	#define makePixel(r, g, b) PocoMakePixelCLUT16(r, g, b)
#elif kCommodettoBitmapGray16 == kPocoPixelFormat
	#define makePixel(r, g, b) PocoMakePixelGray16(r, g, b)
#else
	#error
#endif

void convertto_4(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels)
{
	int i, pixelCount;
	int outWidth = cb->w;
	int outHeight = cb->h;

	if (PJPG_YH1V1 == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i += 2) {
				*pixels++ = (makePixel(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) |
							makePixel(jpeg->r[i + 1], jpeg->g[i + 1], jpeg->b[i + 1]);;
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x += 2) {
					if ((outWidth - x) >= 2) {
						*pixels++ = (makePixel(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) |
									makePixel(jpeg->r[i + 1], jpeg->g[i + 1], jpeg->b[i + 1]);;
						i += 2;
					}
					else {
						PocoPixel twoPixels = *pixels;
						*pixels++ = (makePixel(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) | (twoPixels & 0x0F);
						i += 1;
					}
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else if (PJPG_YH2V2 == jpeg->scanType) {
		unsigned char *r, *g, *b;
		uint8_t jMaxLeft, jMaxRight;
		int j;

		if (jpeg->mcuWidth == outWidth) {
			jMaxLeft = 8;
			jMaxRight = 72;
		}
		else if (outWidth <= 8) {
			jMaxLeft = outWidth;
			jMaxRight = 0;
		} else {
			jMaxLeft = 8;
			jMaxRight = 56 + outWidth;
		}

		r = jpeg->r, g = jpeg->g, b = jpeg->b;
		for (i = (outHeight >= 8) ? 8 : outHeight; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j += 2)
				*pixels++ = (makePixel(r[j], g[j], b[j]) << 4) |
							makePixel(r[j + 1], g[j + 1], b[j + 1]);;
			for (j = 64; j < jMaxRight; j += 2)
				*pixels++ = (makePixel(r[j], g[j], b[j]) << 4) |
							makePixel(r[j + 1], g[j + 1], b[j + 1]);;
		}

		r = jpeg->r + 128, g = jpeg->g + 128, b = jpeg->b + 128;
		for (i = (outHeight > 8) ? outHeight - 8 : 0; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j += 2)
				*pixels++ = (makePixel(r[j], g[j], b[j]) << 4) |
							makePixel(r[j + 1], g[j + 1], b[j + 1]);;
			for (j = 64; j < jMaxRight; j += 2)
				*pixels++ = (makePixel(r[j], g[j], b[j]) << 4) |
							makePixel(r[j + 1], g[j + 1], b[j + 1]);;
		}
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i += 2) {
				PocoPixel twoPixels;
				PocoPixel gray = ~jpeg->r[i];
				twoPixels = makePixel(gray, gray, gray) << 4;
				gray = ~jpeg->r[i + 1];
				twoPixels |= makePixel(gray, gray, gray);
				*pixels++ = twoPixels;
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x += 2, i += 2) {
					if ((outWidth - x) >= 2) {
						PocoPixel twoPixels = *pixels;
						PocoPixel gray = ~jpeg->r[i];
						twoPixels = makePixel(gray, gray, gray) << 4;
						gray = ~jpeg->r[i + 1];
						twoPixels |= makePixel(gray, gray, gray);
						*pixels ++ = twoPixels;
						i += 2;
					}
					else {
						PocoPixel twoPixels = *pixels;
						PocoPixel gray = ~jpeg->r[i];
						*pixels++ = (makePixel(gray, gray, gray) >> 4) | (twoPixels & 0x0F);
						i += 1;
				}
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else
		modLog("unimplemented scan type");
}
#endif

void convertto_24(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixelsIn)
{
	uint8_t *pixels = (uint8_t *)pixelsIn;
	int i, pixelCount;
	int outWidth = cb->w;
	int outHeight = cb->h;

	if (PJPG_YH1V1 == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++, pixels += 3) {
				pixels[0] = jpeg->r[i];
				pixels[1] = jpeg->g[i];
				pixels[2] = jpeg->b[i];
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++, pixels += 3) {
					pixels[0] = jpeg->r[i];
					pixels[1] = jpeg->g[i];
					pixels[2] = jpeg->b[i];
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else if (PJPG_YH2V2 == jpeg->scanType) {
		unsigned char *r, *g, *b;
		uint8_t jMaxLeft, jMaxRight;
		int j;

		if (jpeg->mcuWidth == outWidth) {
			jMaxLeft = 8;
			jMaxRight = 72;
		}
		else if (outWidth <= 8) {
			jMaxLeft = outWidth;
			jMaxRight = 0;
		} else {
			jMaxLeft = 8;
			jMaxRight = 56 + outWidth;
		}

		r = jpeg->r, g = jpeg->g, b = jpeg->b;
		for (i = (outHeight >= 8) ? 8 : outHeight; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j++, pixels += 3) {
				pixels[0] = r[j];
				pixels[1] = g[j];
				pixels[2] = b[j];
			}
			for (j = 64; j < jMaxRight; j++, pixels += 3) {
				pixels[0] = r[j];
				pixels[1] = g[j];
				pixels[2] = b[j];
			}
		}

		r = jpeg->r + 128, g = jpeg->g + 128, b = jpeg->b + 128;
		for (i = (outHeight > 8) ? outHeight - 8 : 0; i > 0; i--, r += 8, g += 8, b += 8) {
			for (j = 0; j < jMaxLeft; j++, pixels += 3) {
				pixels[0] = r[j];
				pixels[1] = g[j];
				pixels[2] = b[j];
			}
			for (j = 64; j < jMaxRight; j++, pixels += 3) {
				pixels[0] = r[j];
				pixels[1] = g[j];
				pixels[2] = b[j];
			}
		}
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++, pixels += 3) {
				uint8_t gray = ~jpeg->r[i];
				pixels[0] = gray;
				pixels[1] = gray;
				pixels[2] = gray;
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++, pixels += 3) {
					uint8_t gray = ~jpeg->r[i];
					pixels[0] = gray;
					pixels[1] = gray;
					pixels[2] = gray;
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else
		modLog("unimplemented scan type");
}

uint8_t tryInitialize(xsMachine *the, JPEG jpeg)
{
	unsigned char result;
	pjpeg_image_info_t info;
	int pixelsLength;
	int totalBytesAvailable = jpeg->totalBytesAvailable;

	jpeg->bufferIndex = 0;
	activateBuffer(jpeg);

	result = pjpeg_decode_init(&info, needBytes, jpeg, 0);
	if (0 != result) {
		jpeg->totalBytesAvailable = totalBytesAvailable;
		return 1;
	}

	jpeg->blockWidth = info.m_MCUSPerRow;
	jpeg->blockYMax = info.m_MCUSPerCol - 1;
	jpeg->blockX = 0;
	jpeg->blockY = 0;
	jpeg->mcuWidth = info.m_MCUWidth;
	jpeg->mcuHeight = info.m_MCUHeight;
	jpeg->mcuWidthRight = info.m_width % jpeg->mcuWidth;
	if (!jpeg->mcuWidthRight)
		jpeg->mcuWidthRight = jpeg->mcuWidth;
	jpeg->mcuHeightBottom = info.m_height % jpeg->mcuHeight;
	if (!jpeg->mcuHeightBottom)
		jpeg->mcuHeightBottom = jpeg->mcuHeight;
	jpeg->blocksRemaining = info.m_MCUSPerCol * info.m_MCUSPerRow;
	jpeg->scanType = info.m_scanType;
	jpeg->r = info.m_pMCUBufR;
	jpeg->g = info.m_pMCUBufG;
	jpeg->b = info.m_pMCUBufB;

	pixelsLength = (jpeg->mcuWidth * jpeg->mcuHeight * jpeg->pixelSize) >> 3;
	jpeg->pixels = c_malloc(pixelsLength);
	if (!jpeg->pixels)
		xsUnknownError("out of memory");
	xsVar(0) = xsNewHostObject(pixelsDestructor);
	xsmcSetHostBuffer(xsVar(0), jpeg->pixels, pixelsLength);
	xsmcSetInteger(xsVar(1), pixelsLength);
	xsmcDefine(xsVar(0), xsID_byteLength, xsVar(1), xsDefault);
	xsmcSet(xsThis, xsID_pixels, xsVar(0));

	xsmcSetInteger(xsVar(0), info.m_width);
	xsmcSet(xsThis, xsID_width, xsVar(0));
	xsmcSetInteger(xsVar(0), info.m_height);
	xsmcSet(xsThis, xsID_height, xsVar(0));

	xsmcSetInteger(xsVar(0), jpeg->pixelFormat);
	xsCall1(xsThis, xsID_initialize, xsVar(0));

	xsmcGet(xsVar(0), xsThis, xsID_buffers);
	while (jpeg->bufferIndex--)
		xsCall0(xsVar(0), xsID_shift);
	jpeg->bufferIndex = 0;

	return 0;
}

unsigned char needBytes(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data)
{
	JPEG jpeg = pCallback_data;
	const unsigned char *buffer;
	xsMachine *the = jpeg->the;
	xsUnsignedValue srcBytes;

	if (buf_size > (jpeg->bytesInBuffer - jpeg->position)) {
		buf_size = jpeg->bytesInBuffer - jpeg->position;
		if (0 == buf_size)
			return PJPG_STREAM_READ_ERROR;
	}
	jpeg->totalBytesAvailable -= buf_size;

	xsmcGet(xsVar(0), xsThis, xsID_buffers);
	xsmcGetIndex(xsVar(0), xsVar(0), jpeg->bufferIndex);

	xsmcGetBufferReadable(xsVar(0), (void **)&buffer, &srcBytes);
	if (srcBytes < buf_size)
		return PJPG_STREAM_READ_ERROR;

	c_memcpy(pBuf, buffer + jpeg->position, buf_size);

	*pBytes_actually_read = buf_size;
	jpeg->position += buf_size;

	if (jpeg->position == jpeg->bytesInBuffer) {
		xsmcGet(xsVar(0), xsThis, xsID_buffers);
		if (jpeg->r)
			xsCall0(xsVar(0), xsID_shift);
		else
			jpeg->bufferIndex += 1;
		activateBuffer(jpeg);
	}

	return 0;
}

void activateBuffer(JPEG jpeg)
{
	xsMachine *the = jpeg->the;
	void *src;
	xsUnsignedValue srcBytes;

	xsmcGet(xsVar(0), xsThis, xsID_buffers);
	xsmcGetIndex(xsVar(0), xsVar(0), jpeg->bufferIndex);
	if (!xsmcTest(xsVar(0)))
		return;

	jpeg->position = 0;

	xsmcGetBufferReadable(xsVar(0), (void **)&src, &srcBytes);
	jpeg->bytesInBuffer = srcBytes;
}
