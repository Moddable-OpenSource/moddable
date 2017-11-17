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
	Commodetto readJPEG

		wraps picojpeg
		
	To do:

		PJPG_YH2V2

		sliding CommodettoBitmap

*/

#include "stdlib.h"
#include "string.h"

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#ifdef __ets__				//@@ move somewhere else!
	#include "xsesp.h"
#endif

#include "commodettoPocoBlit.h"

#include "picojpeg.h"

#include "picojpeg.c"		//@@

typedef void (*convertto)(void *jpeg, CommodettoBitmap cb, PocoPixel *pixels);

typedef struct {
	int					position;
	int					length;
	xsMachine			*the;
	uint8_t				*pixels;
	convertto			convert;
	char				isArrayBuffer;

	unsigned char		blockWidth;
	unsigned char		blockHeight;
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

	int					lastByteLength;
} JPEGRecord, *JPEG;

static unsigned char needBytes(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);

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
	unsigned char result;
	pjpeg_image_info_t info;
	CommodettoBitmapFormat pixelFormat;
	uint8_t pixelSize;
	int argc = xsmcArgc, pixelsLength;

	JPEG jpeg = c_malloc(sizeof(JPEGRecord));
	if (!jpeg)
		xsErrorPrintf("jpeg out of memory");

	xsmcSetHostData(xsThis, jpeg);

	xsmcVars(2);
	xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
	jpeg->length = xsmcToInteger(xsVar(0));
	jpeg->position = 0;
	jpeg->the = the;

	xsmcSet(xsThis, xsID_buffer, xsArg(0));

 	jpeg->isArrayBuffer = xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype);

	result = pjpeg_decode_init(&info, needBytes, jpeg, 0);
	if (0 != result)
		xsErrorPrintf("jpeg init failed");

	jpeg->blockWidth = info.m_MCUSPerRow;
	jpeg->blockHeight = info.m_MCUSPerCol;
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
	jpeg->scanType = info.m_scanType;
	jpeg->r = info.m_pMCUBufR;
	jpeg->g = info.m_pMCUBufG;
	jpeg->b = info.m_pMCUBufB;

	pixelFormat = kCommodettoBitmapFormat;
	if ((argc > 1) && (xsUndefinedType != xsmcTypeOf(xsArg(1))) && xsmcHas(xsArg(1), xsID_pixelFormat)) {
		xsmcGet(xsVar(0), xsArg(1), xsID_pixelFormat);
		pixelFormat = xsmcToInteger(xsVar(0));
	}

#if (16 == kPocoPixelSize) || (8 == kPocoPixelSize)
	jpeg->convert = (convertto)convertto_16and8;
	pixelSize = kPocoPixelSize;
#endif
#if 4 == kPocoPixelSize
	jpeg->convert = (convertto)convertto_4;
	pixelSize = 4;
#endif
	if (kCommodettoBitmap24RGB == pixelFormat) {
		jpeg->convert = (convertto)convertto_24;
		pixelSize = 24;
	}
	else if (kCommodettoBitmapFormat != pixelFormat)
		xsErrorPrintf("unsupported pixel format");

	pixelsLength = (jpeg->mcuWidth * jpeg->mcuHeight * pixelSize) >> 3;
	jpeg->pixels = c_malloc(pixelsLength);
	if (!jpeg->pixels)
		xsUnknownError("out of memory");
	xsVar(0) = xsNewHostObject(pixelsDestructor);
	xsmcSetHostData(xsVar(0), jpeg->pixels);
	xsmcSetInteger(xsVar(1), pixelsLength);
	xsmcSet(xsVar(0), xsID_byteLength, xsVar(1));
	xsmcSet(xsThis, xsID_pixels, xsVar(0));

	xsVar(0) = xsInteger(info.m_width);
	xsmcSet(xsThis, xsID_width, xsVar(0));
	xsVar(0) = xsInteger(info.m_height);
	xsmcSet(xsThis, xsID_height, xsVar(0));

	xsmcSetInteger(xsVar(0), pixelFormat);
	xsCall1(xsThis, xsID_initialize, xsVar(0));
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
	xsVar(0) = xsInteger(jpeg->blockX * jpeg->mcuWidth);
	xsmcSet(xsVar(1), xsID_x, xsVar(0));
	xsVar(0) = xsInteger(jpeg->blockY * jpeg->mcuHeight);
	xsmcSet(xsVar(1), xsID_y, xsVar(0));

	cb = xsmcGetHostChunk(xsVar(1));
	if (jpeg->blockY >= jpeg->blockHeight)
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
}

#if (kPocoPixelSize == 16) || (kPocoPixelSize == 8)

void convertto_16and8(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels)
{
	int i, pixelCount;
	int outWidth = cb->w;
	int outHeight = cb->h;

	if (PJPG_YH1V1 == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++)
				*pixels++ =	PocoMakeColor(jpeg->r[i], jpeg->g[i], jpeg->b[i]);
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++)
					*pixels++ = PocoMakeColor(jpeg->r[i], jpeg->g[i], jpeg->b[i]);
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else if (PJPG_YH2V2 == jpeg->scanType) {
		;
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++) {
				PocoPixel gray = ~jpeg->r[i];
				*pixels++ = PocoMakeColor(gray, gray, gray);
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x++, i++) {
					PocoPixel gray = ~jpeg->r[i];
					*pixels++ = PocoMakeColor(gray, gray, gray);
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
}
#endif

#if kPocoPixelSize == 4
void convertto_4(JPEG jpeg, CommodettoBitmap cb, PocoPixel *pixels)
{
	int i, pixelCount;
	int outWidth = cb->w;
	int outHeight = cb->h;

	if (PJPG_YH1V1 == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i += 2) {
				*pixels++ = (PocoMakeColor(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) |
							PocoMakeColor(jpeg->r[i + 1], jpeg->g[i + 1], jpeg->b[i + 1]);;
			}
		}
		else {									// horizontal clipping (right edge)
			int x, y;
			i = 0;
			for (y = 0; y < outHeight; y++) {
				for (x = 0; x < outWidth; x += 2) {
					if ((outWidth - x) >= 2) {
						*pixels++ = (PocoMakeColor(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) |
									PocoMakeColor(jpeg->r[i + 1], jpeg->g[i + 1], jpeg->b[i + 1]);;
						i += 2;
					}
					else {
						PocoPixel twoPixels = *pixels;
						*pixels++ = (PocoMakeColor(jpeg->r[i], jpeg->g[i], jpeg->b[i]) << 4) | (twoPixels & 0x0F);
						i += 1;
					}
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
	else if (PJPG_YH2V2 == jpeg->scanType) {
		;
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i += 2) {
				PocoPixel twoPixels;
				PocoPixel gray = ~jpeg->r[i];
				twoPixels = PocoMakeColor(gray, gray, gray) << 4;
				gray = ~jpeg->r[i + 1];
				twoPixels |= PocoMakeColor(gray, gray, gray);
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
						twoPixels = PocoMakeColor(gray, gray, gray) << 4;
						gray = ~jpeg->r[i + 1];
						twoPixels |= PocoMakeColor(gray, gray, gray);
						*pixels ++ = twoPixels;
						i += 2;
					}
					else {
						PocoPixel twoPixels = *pixels;
						PocoPixel gray = ~jpeg->r[i];
						*pixels++ = (PocoMakeColor(gray, gray, gray) >> 4) | (twoPixels & 0x0F);
						i += 1;
				}
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
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
		;
	}
	else if (PJPG_GRAYSCALE == jpeg->scanType) {
		if (jpeg->mcuWidth == outWidth) {		// no horizontal clipping
			for (i = 0, pixelCount = jpeg->mcuWidth * outHeight; i < pixelCount; i++, pixels += 3) {
				PocoPixel gray = ~jpeg->r[i];
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
					PocoPixel gray = ~jpeg->r[i];
					pixels[0] = gray;
					pixels[1] = gray;
					pixels[2] = gray;
				}
				i += jpeg->mcuWidth - outWidth;
			}
		}
	}
}

unsigned char needBytes(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data)
{
	JPEG jpeg = pCallback_data;
	const unsigned char *buffer;
	xsMachine *the = jpeg->the;

	if (buf_size > (jpeg->length - jpeg->position))
		buf_size = jpeg->length - jpeg->position;

	xsmcGet(xsVar(0), xsThis, xsID_buffer);
	if (jpeg->isArrayBuffer)
		buffer = xsmcToArrayBuffer(xsVar(0));
	else
		buffer = xsmcGetHostData(xsVar(0));

	c_memcpy(pBuf, buffer + jpeg->position, buf_size);

	*pBytes_actually_read = buf_size;
	jpeg->position += buf_size;

	return 0;
}
