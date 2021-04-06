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

// ick
#include "AnimatedGIF.c"

static void doDraw(GIFDRAW *pDraw);

// scratch globals because gif library doesn't have way to pass state to callbacks
static uint16_t *gBitmap;
static uint16_t gBitmapStride;

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
	int32_t bufferSize;
	void *buffer;

	xsmcVars(1);
	buffer = xsmcGetHostData(xsArg(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
	bufferSize = xsmcToInteger(xsVar(0));

	xg = xsmcGetHostChunk(xsThis);
    xg->bufferSize = bufferSize;

	GIFIMAGE *pGIF = &xg->gi;
    pGIF->iError = GIF_SUCCESS;
    pGIF->ucPaletteType = GIF_PALETTE_RGB565;
	pGIF->ucLittleEndian = true;
    pGIF->pfnRead = readMem;
    pGIF->pfnSeek = seekMem;
    pGIF->pfnDraw = doDraw;
    pGIF->pfnOpen = NULL;
    pGIF->pfnClose = NULL;
    pGIF->GIFFile.iSize = bufferSize;
    pGIF->GIFFile.pData = buffer;
    GIFInit(pGIF);
    
	xg->bitmap.w = pGIF->iCanvasWidth;
	xg->bitmap.h = pGIF->iCanvasHeight;
	xg->bitmap.format = kCommodettoBitmapRGB565LE;
	xg->bitmap.havePointer = 1;
	xg->bitmap.bits.data = c_malloc(2 * pGIF->iCanvasWidth * pGIF->iCanvasHeight);
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
		if (GIFGetInfo(&xg->gi, NULL))
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
		if (pGIF->bHasGlobalColorTable) {
			uint16_t *palette = (uint16_t *)pGIF->pPalette;
			xg->background = palette[pGIF->ucBackground];
			xg->hasTransparentBackground = (pGIF->ucBackground == pGIF->ucTransparent) && (pGIF->ucGIFBits & 1);
		}
		else
			xg->background = 0xFFFF;		// white
	}

	if (xg->hasTransparentBackground && xg->hasTransparentColor)
		xg->background = xg->transparentColor;

	if (2 == xg->disposalMethod) {
		uint16_t *dst = ((xg->prevY * pGIF->iCanvasWidth) + xg->prevX) + (uint16_t *)xg->bitmap.bits.data;
		int h = xg->prevH;
		uint16_t background = xg->background;
		int skip = pGIF->iCanvasWidth - xg->prevW;

		do {
			int w = xg->prevW;

			do {
				*dst++ = background;
			}  while (--w);

			dst += skip;
		} while (--h);
	}

	gBitmap = ((pGIF->iY * pGIF->iCanvasWidth) + pGIF->iX) + (uint16_t *)xg->bitmap.bits.data;
	gBitmapStride = pGIF->iCanvasWidth;
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
	xsmcSetInteger(xsResult, xg->gi.iCanvasWidth);
}

void xs_readgif_get_height(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, xg->gi.iCanvasHeight);
}

void xs_readgif_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapRGB565LE);
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
		if (xg->reduced || !GIFGetInfo(&xg->gi, &xg->ginfo))
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
	xsDefine(xsResult, xsID_x, xsInteger(frameX), xsDefault);
	xsDefine(xsResult, xsID_y, xsInteger(frameY), xsDefault);
	xsDefine(xsResult, xsID_width, xsInteger(frameW), xsDefault);
	xsDefine(xsResult, xsID_height, xsInteger(frameH), xsDefault);
}

void xs_readgif_get_frameCount(xsMachine *the)
{
	xsGIF xg = xsmcGetHostChunk(xsThis);
	if (2 != xg->ready) {
		if (!GIFGetInfo(&xg->gi, &xg->ginfo))
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
	if (xg->hasTransparentColor)
		xsmcSetInteger(xsResult, xg->transparentColor);
}

void xs_readgif_set_transparentColor(xsMachine *the)
{
	int transparentColor = xsmcToInteger(xsArg(0));
	xsGIF xg = xsmcGetHostChunk(xsThis);
	uint16_t prev = xg->transparentColor;
	xg->transparentColor = (uint16_t)transparentColor;
	xg->hasTransparentColor = true;

	uint16_t *pixels = xg->bitmap.bits.data;
	int i = xg->gi.iCanvasWidth * xg->gi.iCanvasHeight;
	do {
		if (*pixels == prev)
			*pixels = (uint16_t)transparentColor;
		pixels++;
	} while (--i);
}

// caller ensures pDraw->pPixels is 32-bit aligned
void doDraw(GIFDRAW *pDraw)
{
	uint16_t *dst = gBitmap + (pDraw->y * gBitmapStride);
	uint16_t *palette = (uint16_t *)pDraw->pPalette;
	uint8_t *src = pDraw->pPixels;
	uint16_t i = pDraw->iWidth;

	if (pDraw->ucHasTransparency) {
		uint8_t ucTransparent = pDraw->ucTransparent;
		while (i >= 4)  {
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

		while (i--)  {
			uint8_t index = *src++;
			if (index != ucTransparent)
				*dst = palette[index];
			dst++;
		}
	}
	else {
		while (i >= 4)  {
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
