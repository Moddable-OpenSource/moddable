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
#include "mc.xs.h"
#include "xsHost.h"
#include "commodettoReadGIF.h"
#include "mc.defines.h"

// ick
#include "AnimatedGIF.c"

static void doDraw565LE(GIFDRAW *pDraw);
static void doDrawCLUT256(GIFDRAW *pDraw);
static void doDrawGray16(GIFDRAW *pDraw);
static void doDrawMonochrome(GIFDRAW *pDraw);
static void doDrawCLUT32(GIFDRAW *pDraw);

// scratch globals because gif library doesn't have way to pass state to callbacks
static uint16_t *gBitmap;
static uint16_t gBitmapStride;
static uint8_t gBitmapPhase;

#define Draw5BitPixel(color, phase, dst)	\
	switch (phase++) {	\
		case 0:		\
			dst[0] = (dst[0] & ~0xF8) | (color << 3);	\
			break;	\
					\
		case 1:		\
			dst[0] = (dst[0] & ~0x07) | (color >> 2);	\
			dst[1] = (dst[1] & ~0xC0) | (color << 6);	\
			break;	\
					\
		case 2:		\
			dst[1] = (dst[1] & ~0x3E) | (color << 1);	\
			break;	\
					\
		case 3:		\
			dst[1] = (dst[1] & ~0x01) | (color >> 4);	\
			dst[2] = (dst[2] & ~0xF0) | (color << 4);	\
			break;	\
					\
		case 4:		\
			dst[2] = (dst[2] & ~0x0F) | (color >> 1);	\
			dst[3] = (dst[3] & ~0x80) | (color << 7);	\
			break;	\
					\
		case 5:		\
			dst[3] = (dst[3] & ~0x7C) | (color << 2);	\
			break;	\
					\
		case 6:		\
			dst[3] = (dst[3] & ~0x03) | (color >> 3);	\
			dst[4] = (dst[4] & ~0xE0) | (color << 5);	\
			break;	\
					\
		case 7:		\
			dst[4] = (dst[4] & ~0x1F) | color;			\
			dst += 5;									\
			phase = 0;									\
			break;										\
	}

void xs_readgif_destructor(void *data)
{
	xsGIF xg = data;

	if (xg && xg->bitmap.bits.data) {
		c_free(xg->bitmap.bits.data);
		xg->bitmap.bits.data = NULL;
	}
}

void xs_readgif(xsMachine *the)
{
	xsGIF xg = xsmcSetHostChunk(xsThis, NULL, sizeof(xsGIFRecord));
	void *buffer;
	xsUnsignedValue bufferSize;
	xsIntegerValue available;
	int format = kCommodettoBitmapDefault;

	xsmcGetBufferReadable(xsArg(0), &buffer, &bufferSize);
	available = (xsIntegerValue)bufferSize;

	xg = xsmcGetHostChunk(xsThis);
	xg->bufferSize = bufferSize;

	if (xsmcArgc > 1) {
		xsmcVars(1);

		if (xsmcHas(xsArg(1), xsID_available)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_available);
			xsUnsignedValue available = xsmcToInteger(xsVar(0));
			if ((available < 0) || (available > bufferSize))
				xsUnknownError("invalid");
		}

		if (xsmcHas(xsArg(1), xsID_pixelFormat)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_pixelFormat);
			format = xsmcToInteger(xsVar(0));
			if ((kCommodettoBitmapRGB565LE != format) && (kCommodettoBitmapCLUT256 != format)
				&& (kCommodettoBitmapGray16 != format) && (kCommodettoBitmapMonochrome != format)
				&& (kCommodettoBitmapCLUT32 != format))
				xsUnknownError("unsupported format");
		}
	}

	GIFIMAGE *pGIF = &xg->gi;
	pGIF->iError = GIF_SUCCESS;
	pGIF->ucPaletteType = GIF_PALETTE_RGB565;
	pGIF->ucLittleEndian = true;
	pGIF->pfnRead = readMem;
	pGIF->pfnSeek = seekMem;
	pGIF->pfnOpen = NULL;
	pGIF->pfnClose = NULL;
	pGIF->GIFFile.iSize = available;
	pGIF->GIFFile.pData = buffer;
	GIFInit(pGIF);

	if (((xsUnsignedValue)available == bufferSize) && (kCommodettoBitmapDefault == format)) {
		if (GIFGetInfo(&xg->gi, &xg->ginfo, xg->gi.pPalette)) {
			xg->ready = 2;

			if (xg->ginfo.sGlobalColorTableSize && !xg->ginfo.ucHasLocalColorTable) {
				uint16_t monochrome = 0;
				uint32_t gray16 = 0;
				int i, gray = 1;
				for (i = 0; i < xg->ginfo.sGlobalColorTableSize; i++) {
					uint16_t color = xg->gi.pPalette[i];
					int r = color >> 11;
					int g = (color >> 6) & 0x1F;
					int b = color & 0x1F;
					if ((r == g) && (r == b)) {
						if (0 == r)
							monochrome |= 1;
						else if (31 == r)
							monochrome |= 2;
						else
							monochrome |= 4;

						gray16 |= 1 << r;
					}
					else {
						gray = 0;
						monochrome |= 4;
						break;
					}
				}
				if ((3 == monochrome) && !xg->ginfo.ucHasTransparent)
					format = kCommodettoBitmapMonochrome;
				else if ((gray16 & 0xaaaa5555) && !(gray16 & 0x5555aaaa) && gray && !xg->ginfo.ucHasTransparent)
					format = kCommodettoBitmapGray16;
				else if (xg->ginfo.sGlobalColorTableSize <= 32)
					format = kCommodettoBitmapCLUT32;
				else
					format = kCommodettoBitmapCLUT256;
			}
		}
		(*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // rewind
	}

	if (kCommodettoBitmapDefault == format)
		format = kCommodettoBitmapRGB565LE;

	xg->bitmap.w = pGIF->iCanvasWidth;
	xg->bitmap.h = pGIF->iCanvasHeight;
	if (kCommodettoBitmapMonochrome == format)
		xg->bitmap.w = (xg->bitmap.w + 7) & ~7;
	else if (kCommodettoBitmapGray16 == format)
		xg->bitmap.w = (xg->bitmap.w + 1) & ~1;

	xg->bitmap.havePointer = 1;
	xg->bitmap.format = (CommodettoBitmapFormat)format;
	if (kCommodettoBitmapRGB565LE == format) {
		xg->bitmap.bits.data = c_malloc(2 * xg->bitmap.w * pGIF->iCanvasHeight);
		xg->pixels = xg->bitmap.bits.data;
		pGIF->pfnDraw = doDraw565LE;
	}
	else if (kCommodettoBitmapCLUT256 == format) {
		xg->bitmap.bits.data = c_malloc((258 * sizeof(uint16_t)) + (xg->bitmap.w * pGIF->iCanvasHeight));
		if (xg->bitmap.bits.data) {
			uint16_t *t = (uint16_t *)xg->bitmap.bits.data;
			t[0] = 0xaa55;
			t[1] = 0;
			xg->pixels = &t[2];
			pGIF->pfnDraw = doDrawCLUT256;
		}
	} else if (kCommodettoBitmapGray16 == format) {
		xg->bitmap.bits.data = c_malloc(((xg->bitmap.w + 1) >> 1) * pGIF->iCanvasHeight);
		xg->pixels = xg->bitmap.bits.data;
		pGIF->pfnDraw = doDrawGray16;
	} else if (kCommodettoBitmapMonochrome == format) {
		xg->bitmap.bits.data = c_malloc(((xg->bitmap.w + 7) >> 3) * pGIF->iCanvasHeight);
		xg->pixels = xg->bitmap.bits.data;
		pGIF->pfnDraw = doDrawMonochrome;
	} else if (kCommodettoBitmapCLUT32 == format) {
		xg->bitmap.bits.data = c_malloc((34 * sizeof(uint16_t)) + (((xg->bitmap.w + 7) >> 3) * 5 * pGIF->iCanvasHeight));
		if (xg->bitmap.bits.data) {
			uint16_t *t = (uint16_t *)xg->bitmap.bits.data;
			t[0] = 0xaa55;
			t[1] = 0;
			xg->pixels = &t[2];
			pGIF->pfnDraw = doDrawCLUT32;
		}
	}
	if (NULL == xg->bitmap.bits.data)
		xsUnknownError("no memory");

	xg->frameNumber = -1;
}

void xs_readgif_close(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xs_readgif_destructor(xg);
	xsmcSetHostData(xsThis, NULL);
}

void xs_readgif_get_ready(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);

	if (xg->reduced)
		return;

	if (!xg->ready) {
		if (GIFGetInfo(&xg->gi, NULL, NULL))
			xg->ready = 1;
	}

	xsmcSetBoolean(xsResult, xg->ready ? 1 : 0);
}

void xs_readgif_set_available(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	int32_t iSize = xsmcToInteger(xsArg(0));
	if ((iSize < 0) || (iSize > xg->bufferSize))
		xsUnknownError("out of range");
	if (xg->reduced)
		return;
	xg->gi.GIFFile.iSize = iSize;
}

void xs_readgif_first(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	GIFIMAGE *pGIF = &xg->gi;

	if (xg->reduced)
		return;

	(*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // rewind
	xg->frameNumber = -1;
	xsCall0(xsThis, xsID_next);
}

void xs_readgif_next(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	GIFIMAGE *pGIF = &xg->gi;
	uint16_t x, y, w, h;
	uint8_t first = -1 == xg->frameNumber;

	if (xg->reduced)
		return;

	if (pGIF->GIFFile.iPos >= pGIF->GIFFile.iSize - 1) {	// no more data
		(*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // rewind
		xg->frameNumber = 0;
	}
	else
		xg->frameNumber += 1;

	if (0 == xg->frameNumber) {
		xg->prevX = 0;
		xg->prevY = 0;
		xg->prevW = pGIF->iCanvasWidth;
		xg->prevH = pGIF->iCanvasHeight;
		xg->disposalMethod = 2;
	}

	x = xg->prevX, y = xg->prevY, w = xg->prevW, h = xg->prevH;

	GIFParseInfo(pGIF, 0);
	if (GIF_EARLY_EOF == pGIF->iError) {
		(*pGIF->pfnSeek)(&pGIF->GIFFile, 0); // rewind
		xg->frameNumber = 0;

		pGIF->iError = GIF_SUCCESS;
		GIFInit(pGIF);
		GIFParseInfo(pGIF, 0);
	}

 	if (GIF_SUCCESS != pGIF->iError)
		return;

	if (first) {
		if (pGIF->sGlobalColorTableCount) {
			uint16_t *palette = (uint16_t *)pGIF->pPalette;
			xg->background = palette[pGIF->ucBackground];
			xg->hasTransparentBackground = (pGIF->ucBackground == pGIF->ucTransparent) && (pGIF->ucGIFBits & 1);

			if ((kCommodettoBitmapCLUT256 == xg->bitmap.format) || (kCommodettoBitmapCLUT32 == xg->bitmap.format)) {
				uint16_t *t = (uint16_t *)xg->bitmap.bits.data;
				t[1] = pGIF->sGlobalColorTableCount;
				c_memcpy(t + 2, palette, sizeof(uint16_t) * pGIF->sGlobalColorTableCount);
				xg->pixels = &t[2 + pGIF->sGlobalColorTableCount];
#if MODDEF_GIF_BOOST
				uint16_t *c = t + 2;
				int i;
				for (i = 0; i < pGIF->sGlobalColorTableCount; i++) {
					uint16_t pixel = *c;
					int r = (pixel >> 11) & 0x1F;
					int g = (pixel >> 5) & 0x3F;
					int b = (pixel >> 0) & 0x1F;
					r += MODDEF_GIF_BOOST; if (r > 31) r = 31;
					g += MODDEF_GIF_BOOST << 1; if (g > 63) g = 63;
					b += MODDEF_GIF_BOOST; if (b > 31) b = 31;
					*c++ = (r << 11) | (g << 5) | b;
				}
#endif
			}
		}
		else
			xg->background = 0xFFFF;		// white
	}

	if (xg->hasTransparentBackground && xg->hasTransparentColor)
		xg->background = xg->transparentColor;

	if (2 == xg->disposalMethod) {
		if (kCommodettoBitmapRGB565LE == xg->bitmap.format) {
			uint16_t *dst = ((xg->prevY * xg->bitmap.w) + xg->prevX) + (uint16_t *)xg->pixels;
			int h = xg->prevH;
			uint16_t background = xg->background;
			int skip = xg->bitmap.w - xg->prevW;

			do {
				int w = xg->prevW;

				do {
					*dst++ = background;
				} while (--w);

				dst += skip;
			} while (--h);
		}
		else if (kCommodettoBitmapCLUT256 == xg->bitmap.format) {
			uint8_t *dst = ((xg->prevY * xg->bitmap.w) + xg->prevX) + (uint8_t *)xg->pixels;
			int h = xg->prevH;
			uint8_t background = pGIF->ucBackground;		//@@ incorrect if background was changed... but does that matter here?
			int skip = xg->bitmap.w - xg->prevW;

			do {
				int w = xg->prevW;

				do {
					*dst++ = background;
				} while (--w);

				dst += skip;
			} while (--h);
		}
		else if (kCommodettoBitmapGray16 == xg->bitmap.format) {
			uint8_t phase = xg->prevX & 1;
			uint8_t *dst = ((xg->prevY * ((xg->bitmap.w + 1) >> 1)) + (xg->prevX >> 1)) + (uint8_t *)xg->pixels;
			int h = xg->prevH;
			uint8_t background = pGIF->ucBackground;		//@@ incorrect if background was changed... but does that matter here?
			int skip = (xg->bitmap.w - xg->prevW) >> 1;

			do {
				int w = xg->prevW;

				if (phase) {
					*dst = (*dst & 0xF0) | background;
					w--;
				}
				while (w >= 2) {
					w -= 2;
					*dst++ = background | (background << 4);
				}
				if (w)
					*dst = (*dst & 0x0F) | (background << 4);

				dst += skip;
			} while (--h);
		}
		else if (kCommodettoBitmapMonochrome == xg->bitmap.format) {
			uint8_t mask = 1 << (~xg->prevX & 7);
			uint8_t *dst = ((xg->prevY * ((xg->bitmap.w + 7) >> 3)) + (xg->prevX >> 3)) + (uint8_t *)xg->pixels;
			int h = xg->prevH;
			int skip = (xg->bitmap.w - xg->prevW) >> 3;

			do {
				int w = xg->prevW;
				uint8_t pixels = *dst;
				do {
					pixels &= ~mask;

					mask >>= 1;
					if (!mask) {
						dst[0] = pixels;
						pixels = dst[1];
						dst += 1;
						mask = 0x80;
					}
				} while (--w);

				if (0x80 != mask)
					*dst = pixels;

				dst += skip;
			} while (--h);
		}
		else if (kCommodettoBitmapCLUT32 == xg->bitmap.format) {
			uint16_t rowBytes = ((xg->bitmap.w + 7) >> 3) * 5;
			uint8_t *dst = (xg->prevY * rowBytes) + ((xg->prevX >> 3) * 5) + (uint8_t *)xg->pixels;
			int h = xg->prevH;
			uint8_t background = pGIF->ucBackground;		//@@ incorrect if background was changed... but does that matter here?

			do {
				int w = xg->prevW;
				uint8_t *d = dst;
				uint8_t phase = xg->prevX & 7;

				do {
					Draw5BitPixel(background, phase, d);
				} while (--w);

				dst += rowBytes;
			} while (--h);
		}
	}

	gBitmapStride = xg->bitmap.w;
	if (kCommodettoBitmapRGB565LE == xg->bitmap.format)
		gBitmap = ((pGIF->iY * xg->bitmap.w) + pGIF->iX) + (uint16_t *)xg->pixels;
	else if (kCommodettoBitmapCLUT256 == xg->bitmap.format)
		gBitmap = (uint16_t *)(((pGIF->iY * gBitmapStride) + pGIF->iX) + (uint8_t *)xg->pixels);
	else if (kCommodettoBitmapGray16 == xg->bitmap.format) {
		gBitmapStride = (gBitmapStride + 1) >> 1;
		gBitmap = (uint16_t *)(((pGIF->iY * gBitmapStride) + (pGIF->iX >> 1)) + (uint8_t *)xg->pixels);
		gBitmapPhase = pGIF->iX & 1;
	}
	else if (kCommodettoBitmapMonochrome == xg->bitmap.format) {
		gBitmapStride = (gBitmapStride + 7) >> 3;
		gBitmap = (uint16_t *)(((pGIF->iY * gBitmapStride) + (pGIF->iX >> 3)) + (uint8_t *)xg->pixels);
		gBitmapPhase = 1 << (~pGIF->iX & 7);
	}
	else if (kCommodettoBitmapCLUT32 == xg->bitmap.format) {
		gBitmapStride = ((gBitmapStride + 7) >> 3) * 5;
		gBitmap = (uint16_t *)(((pGIF->iY * gBitmapStride) + ((pGIF->iX >> 3) * 5)) + (uint8_t *)xg->pixels);
		gBitmapPhase = pGIF->iX & 7;
	}
	int rc = DecodeLZW(pGIF, 0);
	gBitmap = NULL;
	if (rc != 0) // problem
		xsUnknownError("decode failed");

	xg->prevX = pGIF->iX;
	xg->prevY = pGIF->iY;
	xg->prevW = pGIF->iWidth;
	xg->prevH = pGIF->iHeight;
	if (2 == xg->disposalMethod) {
		// union previous and current
		if (xg->prevX < x) {
			w += x -xg->prevX;
			x = xg->prevX;
		}
		if (xg->prevY < y) {
			h += y -xg->prevY;
			y = xg->prevY;
		}
		if ((xg->prevX + xg->prevW) > (x + w))
			w = (xg->prevX + xg->prevW) - x;
		if ((xg->prevY + xg->prevH) > (y + h))
			h = (xg->prevY + xg->prevH) - y;
		xg->frameX = x;
		xg->frameY = y;
		xg->frameW = w;
		xg->frameH = h;
	}	
	else {
		xg->frameX = xg->prevX;
		xg->frameY = xg->prevY;
		xg->frameW = xg->prevW;
		xg->frameH = xg->prevH;
	}
	
	xg->disposalMethod = (pGIF->ucGIFBits & 0x1c) >> 2;

	// if this GIF has just one image, toss decoding state to reduce memory use
	if ((0 == xg->frameNumber) && (pGIF->GIFFile.iPos >= pGIF->GIFFile.iSize - 1)) {
		void *tmp = c_malloc(offsetof(xsGIFRecord, disposalMethod));
		if (!tmp) return;

		xg->ready = true;
		xg->reduced = true;
		c_memmove(tmp, xg, offsetof(xsGIFRecord, disposalMethod));
		xsmcSetHostData(xsThis, NULL);		// releases current chunk
		xsmcSetHostChunk(xsThis, tmp, offsetof(xsGIFRecord, disposalMethod));
		c_free(tmp);
	}
}

// bitmap

void xs_readgif_get_width(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->bitmap.w);
}

void xs_readgif_get_height(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->bitmap.h);
}

void xs_readgif_get_pixelFormat(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->bitmap.format);
}

void xs_readgif_get_offset(xsMachine *the)
{
	xsmcSetInteger(xsResult, 0);
}

void xs_readgif_get_depth(xsMachine *the)
{
	xsmcSetInteger(xsResult, 16);
}

// animation

void xs_readgif_get_duration(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);

	if (2 != xg->ready) {
		if (xg->reduced || !GIFGetInfo(&xg->gi, &xg->ginfo, NULL))
			return;
		xg->ready = 2;
	}
	xsmcSetInteger(xsResult, xg->ginfo.iDuration);
}

void xs_readgif_get_frameBounds(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	uint16_t frameX = xg->frameX;
	uint16_t frameY = xg->frameY;;
	uint16_t frameW = xg->frameW;;
	uint16_t frameH = xg->frameH;;
	xsmcSetNewObject(xsResult);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), frameX);
	xsmcDefine(xsResult, xsID_x, xsVar(0), xsDefault);
	xsmcSetInteger(xsVar(0), frameY);
	xsmcDefine(xsResult, xsID_y, xsVar(0), xsDefault);
	xsmcSetInteger(xsVar(0), frameW);
	xsmcDefine(xsResult, xsID_width, xsVar(0), xsDefault);
	xsmcSetInteger(xsVar(0), frameH);
	xsmcDefine(xsResult, xsID_height, xsVar(0), xsDefault);
}

void xs_readgif_get_frameCount(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	if (2 != xg->ready) {
		if (!GIFGetInfo(&xg->gi, &xg->ginfo, NULL))
			return;
		xg->ready = 2;
	}
	xsmcSetInteger(xsResult, xg->ginfo.iFrameCount);
}

void xs_readgif_get_frameDuration(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->gi.iFrameDelay);
}

void xs_readgif_get_frameNumber(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->frameNumber);
}

void xs_readgif_get_frameX(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->frameX);
}

void xs_readgif_get_frameY(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->frameY);
}

void xs_readgif_get_frameWidth(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->frameW);
}

void xs_readgif_get_frameHeight(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->frameH);
}

// transparency

void xs_readgif_get_transparent(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetBoolean(xsResult, xg->hasTransparentBackground);
}

void xs_readgif_get_transparentColor(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	if ((kCommodettoBitmapCLUT256 == xg->bitmap.format) || (kCommodettoBitmapCLUT32 == xg->bitmap.format)) {
		GIFIMAGE *pGIF = &xg->gi;
		if (xg->hasTransparentBackground)
			xsmcSetInteger(xsResult, pGIF->ucBackground);
	}
	else  {
		if (xg->hasTransparentBackground)
			xsmcSetInteger(xsResult, xg->background);
	}
}

void xs_readgif_set_transparentColor(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);

	if (kCommodettoBitmapRGB565LE != xg->bitmap.format)
		return;

	int transparentColor = xsmcToInteger(xsArg(0));
	uint16_t prev = xg->transparentColor;
	xg->transparentColor = (uint16_t)transparentColor;
	xg->hasTransparentColor = true;

	uint16_t *pixels = xg->pixels;
	int i = xg->bitmap.w * xg->gi.iCanvasHeight;
	do {
		if (*pixels == prev)
			*pixels = (uint16_t)transparentColor;
		pixels++;
	} while (--i);
}

// caller ensures pDraw->pPixels is 32-bit aligned
void doDraw565LE(GIFDRAW *pDraw)
{
	uint16_t *dst = gBitmap + (pDraw->y * gBitmapStride);
	uint16_t *palette = (uint16_t *)pDraw->pPalette;
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;

	if (pDraw->ucHasTransparency) {
		uint8_t ucTransparent = pDraw->ucTransparent;
		while (i >= 4) {
			uint8_t index;
			uint32_t pixels = *(uint32_t *)src;
			i -= 4;
			src += 4;

			index = (uint8_t)pixels;
			if (index != ucTransparent)
				dst[0] = palette[index];

			index = (uint8_t)(pixels >> 8);
			if (index != ucTransparent)
				dst[1] = palette[index];

			index = (uint8_t)(pixels >> 16);
			if (index != ucTransparent)
				dst[2] = palette[index];

			index = (uint8_t)(pixels >> 24);
			if (index != ucTransparent)
				dst[3] = palette[index];

			dst += 4;
		}

		while (i--) {
			uint8_t index = *src++;
			if (index != ucTransparent)
				*dst = palette[index];
			dst++;
		}
	}
	else {
		while (i >= 4) {
			uint32_t pixels = *(uint32_t *)src;
			i -= 4;
			src += 4;

			dst[0] = palette[(uint8_t)pixels];
			dst[1] = palette[(uint8_t)(pixels >> 8)];
			dst[2] = palette[(uint8_t)(pixels >> 16)];
			dst[3] = palette[(uint8_t)(pixels >> 24)];

			dst += 4;
		}

		while (i--)
			*dst++ = palette[*src++];
	}
}

void doDrawCLUT256(GIFDRAW *pDraw)
{
	uint8_t *dst = ((uint8_t *)gBitmap) + (pDraw->y * gBitmapStride);
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;

	if (pDraw->ucHasTransparency) {
		uint8_t ucTransparent = pDraw->ucTransparent;
		while (i >= 4) {
			uint8_t index;
			uint32_t pixels = *(uint32_t *)src;
			i -= 4;
			src += 4;

			index = (uint8_t)pixels;
			if (index != ucTransparent)
				dst[0] = index;

			index = (uint8_t)(pixels >> 8);
			if (index != ucTransparent)
				dst[1] = index;

			index = (uint8_t)(pixels >> 16);
			if (index != ucTransparent)
				dst[2] = index;

			index = (uint8_t)(pixels >> 24);
			if (index != ucTransparent)
				dst[3] = index;

			dst += 4;
		}

		while (i--) {
			uint8_t index = *src++;
			if (index != ucTransparent)
				*dst = index;
			dst++;
		}
	}
	else {
		while (i >= 4) {
			*(uint32_t *)dst = *(uint32_t *)src;
			i -= 4;
			src += 4;
			dst += 4;
		}

		while (i--)
			*dst++ = *src++;
	}
}

// no transparency in gray16 @@ fix that !!
void doDrawGray16(GIFDRAW *pDraw)
{
	uint8_t *dst = ((uint8_t *)gBitmap) + (pDraw->y * gBitmapStride);
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;
	uint16_t *palette = (uint16_t *)pDraw->pPalette;

	if (gBitmapPhase) {
		i -= 1;
		*dst = (*dst & 0xF0) | (~(palette[*src++] >> 1) & 0x0F);
		dst += 1;
	}
	while (i >= 2) {
		i -= 2;
		*dst++ = ~(((palette[src[0]] & 0x1E) << 3) | ((palette[src[1]] >> 1) & 0x0F));
		src += 2;
	}
	if (i)
		*dst = (*dst & 0x0F) | ((~palette[src[0]] & 0x1E) << 3);
}

// assumes white is index 0 and black is index 1
void doDrawMonochrome(GIFDRAW *pDraw)
{
	uint8_t *dst = ((uint8_t *)gBitmap) + (pDraw->y * gBitmapStride);
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;
	uint8_t mask = gBitmapPhase;
	uint8_t pixels = *dst;

	if (pDraw->ucHasTransparency) {
		uint8_t ucTransparent = pDraw->ucTransparent;
		do {
			uint8_t pixel = *src++;
			if (ucTransparent != pixel) {
				if (pixel)
					pixels |= mask;
				else
					pixels &= ~mask;
			}

			mask >>= 1;
			if (!mask) {
				dst[0] = pixels;
				pixels = dst[1];
				dst += 1;
				mask = 0x80;
			}
		} while (--i);
	}
	else {
		do {
			if (*src++)
				pixels |= mask;
			else
				pixels &= ~mask;

			mask >>= 1;
			if (!mask) {
				dst[0] = pixels;
				pixels = dst[1];
				dst += 1;
				mask = 0x80;
			}
		} while (--i);
	}

	if (0x80 != mask)
		*dst = pixels;
}

void doDrawCLUT32(GIFDRAW *pDraw)
{
	uint8_t *dst = ((uint8_t *)gBitmap) + (pDraw->y * gBitmapStride);
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;
	uint8_t phase = gBitmapPhase;
	uint8_t ucTransparent = pDraw->ucHasTransparency ? pDraw->ucTransparent : 33;

	do {
		uint8_t pixel = *src++;

		if (ucTransparent == pixel) {
			if (phase++ == 7) {
				dst += 5;
				phase = 0;
			}
			continue;
		}

		Draw5BitPixel(pixel, phase, dst);
	} while (--i);
}
