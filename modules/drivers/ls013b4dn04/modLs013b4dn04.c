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

#include "ls013b4dn04.h"
#include "xsmc.h"
#if gecko
	#include "xsPlatform.h"
	#include "xsgecko.h"
#else
	#include "xsesp.h"
#endif
#include "modGPIO.h"
#include "modSPI.h"
#include "commodettoBitmap.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"
#include "mc.defines.h"

#ifndef MODDEF_LS013B4DN04_HZ
	#define MODDEF_LS013B4DN04_HZ 	(10000000)
#endif
#ifndef MODDEF_LS013B4DN04_CS_PORT
	#define MODDEF_LS013B4DN04_CS_PORT	NULL
#endif

static uint8_t gReversedBytes[256] ICACHE_XS6RO_ATTR = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};


#define SCREEN_CS_DEACTIVE	modGPIOWrite(&ls->cs, 0)
#define SCREEN_CS_ACTIVE	modGPIOWrite(&ls->cs, 1)
#define SCREEN_CS_INIT		modGPIOInit(&ls->cs, MODDEF_LS013B4DN04_CS_PORT, MODDEF_LS013B4DN04_CS_PIN, kModGPIOOutput); \
	SCREEN_CS_DEACTIVE

// Host data record.
struct ls013b4dn04Record {
	PixelsOutDispatch dispatch;

	modSPIConfigurationRecord	spiConfig;
	modGPIOConfigurationRecord	cs;

	uint8_t 	onRow;						// store what scanline row we are on

	uint8_t		updateCycle;				// for toggling COM bit to avoid DC Bias

	uint16_t    bytesPerLine;

	uint8_t		*pixelBuffer;
	uint16_t	bufferSize;
};
typedef struct ls013b4dn04Record ls013b4dn04Record;
typedef ls013b4dn04Record *ls013b4dn04;

static void ls013b4dn04ChipSelect(uint8_t active, modSPIConfiguration config);
static void ls_clear(ls013b4dn04 ls);

static uint8_t ls013b4dn04Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void ls013b4dn04Send(PocoPixel *data, int count, void *refCon);
static void ls013b4dn04Continue(void *refcon);
static void ls013b4dn04End(void *refcon);
static void ls013b4dn04AdaptInvalid(void *refcon, CommodettoRectangle invalid);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	(PixelsOutBegin)ls013b4dn04Begin,
	ls013b4dn04End,
	ls013b4dn04End,
	ls013b4dn04Send,
	ls013b4dn04AdaptInvalid
};
#if kCommodettoBitmapFormat != kCommodettoBitmapGray256
	#error gray256 pixels required
#endif

void xs_LS013B4DN04(xsMachine *the){
	ls013b4dn04 ls;

	xsmcVars(1);

	if (kCommodettoBitmapGray256 != kCommodettoBitmapFormat)
		xsUnknownError("gray256 pixels required");

	ls = c_calloc(1, sizeof(ls013b4dn04Record));
	if (!ls)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, ls);
	ls->bytesPerLine = 2 + (MODDEF_LS013B4DN04_WIDTH / 8);
	ls->onRow = 0;
	ls->bufferSize = 0;

	ls->dispatch = (PixelsOutDispatch) &gPixelsOutDispatch;

	SCREEN_CS_INIT;
	modSPIConfig(ls->spiConfig, MODDEF_LS013B4DN04_HZ, MODDEF_LS013B4DN04_SPI_PORT,
			MODDEF_LS013B4DN04_CS_PORT, MODDEF_LS013B4DN04_CS_PIN, ls013b4dn04ChipSelect);
	modSPIInit(&ls->spiConfig);

	ls_clear(ls);
}

uint8_t ls013b4dn04Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h){
	ls013b4dn04 ls = refcon;
	uint16_t xMin = x;
	uint16_t yMin = y;
	uint16_t xMax = xMin + w;
	uint16_t yMax = yMin + h;

	if ((0 != xMin) || (MODDEF_LS013B4DN04_WIDTH != xMax))
		return 1;

	ls->onRow = yMin;

	return 0;
}

void xs_ls013b4dn04_begin(xsMachine *the){
	ls013b4dn04 ls = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	if (ls013b4dn04Begin(ls, x, y, w, h))
		xsUnknownError("partial scan line updates not supported");
}

void ls013b4dn04Send(PocoPixel *data, int count, void *refCon){
	ls013b4dn04 ls = refCon;
	int lines, size;
	uint8_t flags, *toSend;

	modSPIActivateConfiguration(NULL);
	lines = (int)(count / MODDEF_LS013B4DN04_WIDTH);
	size = 2 + (ls->bytesPerLine * lines);

	if (size + 10 > ls->bufferSize){
		if (ls->pixelBuffer) {
			c_free(ls->pixelBuffer);
			ls->pixelBuffer = NULL;
			ls->bufferSize = 0;
		}
		ls->pixelBuffer = c_malloc(size + 10);
		if (!ls->pixelBuffer)
			return;
		ls->bufferSize = (size + 10);
	}

	toSend = ls->pixelBuffer;
	ls->updateCycle = !(ls->updateCycle);
	flags = MODE_FLAG | (ls->updateCycle ? FRAME_FLAG : 0) & (~CLEAR_FLAG);

	for (int i = ls->onRow + 1; i <= (ls->onRow + lines); i++){
		PocoPixel *dest = data + MODDEF_LS013B4DN04_WIDTH;
		*toSend++ = flags;
		*toSend++ = c_read8(gReversedBytes + i);

		while (data < dest){
			*toSend++ = (data[0] & 0x80) | ((data[1] & 0x80) >> 1) | ((data[2] & 0x80) >> 2) | ((data[3] & 0x80) >> 3) | ((data[4] & 0x80) >> 4) | ((data[5] & 0x80) >> 5) | ((data[6] & 0x80) >> 6) | ((data[7] & 0x80) >> 7);
			data += 8;
		}
	}

	ls->onRow = ls->onRow + lines;
	modSPITx(&ls->spiConfig, ls->pixelBuffer, size);
}

void xs_ls013b4dn04_send(xsMachine *the){
	ls013b4dn04 ls = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	const uint8_t *data;
	int count;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		data = xsmcToArrayBuffer(xsArg(0));
		count = xsGetArrayBufferLength(xsArg(0));
	}else {
		xsmcVars(1);
		data = xsmcGetHostData(xsArg(0));
		xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
		count = xsmcToInteger(xsVar(0));
	}

	if (argc > 1) {
		int offset = xsmcToInteger(xsArg(1));
		data += offset;
		count -= offset;
		if (count < 0)
			xsUnknownError("bad offset");
		if (argc > 2) {
			int c = xsmcToInteger(xsArg(2));
			if (c > count)
				xsUnknownError("bad count");
			count = c;
		}
	}

	(ls->dispatch->doSend)((PocoPixel *)data, count, ls);
}

void ls013b4dn04End(void *refcon){
}

void xs_ls013b4dn04_end(xsMachine *the){
  ls013b4dn04End(NULL);
}

void xs_ls013b4dn04_destructor(void *data){
  if (data){
		ls013b4dn04 ls = (ls013b4dn04)data;
		if ( ls->pixelBuffer )
			c_free(ls->pixelBuffer);
		modSPIUninit(&ls->spiConfig);
		c_free(data);
	}
}

void ls013b4dn04Hold(ls013b4dn04 ls){
	ls->updateCycle = !(ls->updateCycle);
	uint8_t mode[6] = {0,0,0,0,0,0};
	if (ls->updateCycle){
		mode[0] |= FRAME_FLAG;
	}
	modSPITx(&ls->spiConfig, mode, 6);
	modSPIActivateConfiguration(NULL);
}

static void ls_clear(ls013b4dn04 ls){
	ls->updateCycle = !(ls->updateCycle);
	uint8_t mode[6] = {0,0,0,0,0,0};
	mode[0] = CLEAR_FLAG;
	if (ls->updateCycle){
		mode[0] |= FRAME_FLAG;
	}

	modSPITx(&ls->spiConfig, mode, 6);
	modSPIActivateConfiguration(NULL);
}

void ls013b4dn04AdaptInvalid(void *refcon, CommodettoRectangle invalid){
	ls013b4dn04 ls = refcon;
	invalid->x = 0;
	invalid->w = MODDEF_LS013B4DN04_WIDTH;
}

void xs_ls013b4dn04_adaptInvalid(xsMachine *the)
{
	ls013b4dn04 ls = xsmcGetHostData(xsThis);
	CommodettoRectangle invalid = xsmcGetHostChunk(xsArg(0));
	ls013b4dn04AdaptInvalid(ls, invalid);
}

void xs_ls013b4dn04_clear(xsMachine *the){
	ls013b4dn04 ls = xsmcGetHostData(xsThis);
	ls_clear(ls);
}

void xs_ls013b4dn04_hold(xsMachine *the){
	ls013b4dn04 ls = xsmcGetHostData(xsThis);
	ls013b4dn04Hold(ls);
}

void xs_ls013b4dn04_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_ls013b4dn04_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_LS013B4DN04_WIDTH);
}

void xs_ls013b4dn04_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_LS013B4DN04_HEIGHT);
}

void xs_ls013b4dn04_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void ls013b4dn04ChipSelect(uint8_t active, modSPIConfiguration config)
{
	ls013b4dn04 ls = (ls013b4dn04)(((char *)config) - offsetof(ls013b4dn04Record, spiConfig));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}
