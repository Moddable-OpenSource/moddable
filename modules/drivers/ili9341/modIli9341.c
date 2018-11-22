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

#include "xsmc.h"

#ifdef __ets__
	#include "xsesp.h"
#elif defined(__ZEPHYR__)
	#include "xsPlatform.h"
	#include "modTimer.h"
#elif defined(gecko)
	#include "xsgecko.h"
	#include "xsPlatform.h"
#else
	#include "xslinux.h"
	#include "xsPlatform.h"
#endif

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "stddef.h"		// for offsetof macro
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "modGPIO.h"
#include "modSPI.h"

#ifndef MODDEF_ILI9341_HZ
	#define MODDEF_ILI9341_HZ (40000000)
#endif
#ifndef MODDEF_ILI9341_FLIPX
	#define MODDEF_ILI9341_FLIPX (false)
#endif
#ifndef MODDEF_ILI9341_FLIPY
	#define MODDEF_ILI9341_FLIPY (false)
#endif
#ifndef MODDEF_ILI9341_CS_PORT
	#define MODDEF_ILI9341_CS_PORT NULL
#endif
#ifndef MODDEF_ILI9341_RST_PORT
	#define MODDEF_ILI9341_RST_PORT NULL
#endif
#ifndef MODDEF_ILI9341_DC_PORT
	#define MODDEF_ILI9341_DC_PORT NULL
#endif

#ifdef MODDEF_ILI9341_CS_PIN
	#define SCREEN_CS_ACTIVE	modGPIOWrite(&sd->cs, 0)
	#define SCREEN_CS_DEACTIVE	modGPIOWrite(&sd->cs, 1)
	#define SCREEN_CS_INIT		modGPIOInit(&sd->cs, MODDEF_ILI9341_CS_PORT, MODDEF_ILI9341_CS_PIN, kModGPIOOutput); \
			SCREEN_CS_DEACTIVE
#else
	#define SCREEN_CS_ACTIVE
	#define SCREEN_CS_DEACTIVE
	#define SCREEN_CS_INIT
#endif

#define SCREEN_DC_DATA			modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&sd->dc, MODDEF_ILI9341_DC_PORT, MODDEF_ILI9341_DC_PIN, kModGPIOOutput); \
		SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, MODDEF_ILI9341_RST_PORT, MODDEF_ILI9341_RST_PIN, kModGPIOOutput); \
		SCREEN_RST_DEACTIVE;

typedef struct {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;

#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	uint8_t						*clut;
#endif

	modGPIOConfigurationRecord	cs;
	modGPIOConfigurationRecord	dc;
#ifdef MODDEF_ILI9341_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif
} spiDisplayRecord, *spiDisplay;

static void ili9341ChipSelect(uint8_t active, modSPIConfiguration config);

static void ili9341Init(spiDisplay sd);
static void ili9341Command(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count);

static void ili9341Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void ili9341End(void *refcon);
static void ili9341AdaptInvalid(void *refcon, CommodettoRectangle r);

#if kCommodettoBitmapFormat == kCommodettoBitmapGray16
static void ili9341Send_Gray16(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_Gray16,
	ili9341AdaptInvalid
};
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
static void ili9341Send_CLUT16(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_CLUT16,
	ili9341AdaptInvalid
};
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
static void ili9341Send_Gray256(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_Gray256,
	NULL
};
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332
static void ili9341Send_RGB332(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_RGB332,
	NULL
};
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB565BE
static void ili9341Send_16BE(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_16BE,
	NULL
};
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
static void ili9341Send_16LE(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341End,
	ili9341End,
	ili9341Send_16LE,
	NULL
};
#endif

void xs_ILI9341_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_ILI9341(xsMachine *the)
{
	spiDisplay sd;

	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		if (kCommodettoBitmapFormat != xsmcToInteger(xsVar(0)))
			xsUnknownError("bad format");
	}

	sd = c_calloc(1, sizeof(spiDisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

	modSPIConfig(sd->spiConfig, MODDEF_ILI9341_HZ, MODDEF_ILI9341_SPI_PORT,
			MODDEF_ILI9341_CS_PORT, MODDEF_ILI9341_CS_PIN, ili9341ChipSelect);

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;

	ili9341Init(sd);
}

void xs_ILI9341_begin(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	ili9341Begin(sd, x, y, w, h);
}

void xs_ILI9341_send(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	const uint8_t *data;
	int count;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		data = xsmcToArrayBuffer(xsArg(0));
		count = xsGetArrayBufferLength(xsArg(0));
	}
	else {
		data = xsmcGetHostData(xsArg(0));

		xsmcVars(1);
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

	(sd->dispatch->doSend)((PocoPixel *)data, count, sd);
}

void xs_ILI9341_end(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	ili9341End(sd);
}

void xs_ILI9341_adaptInvalid(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);

	if (sd->dispatch->doAdaptInvalid) {
		CommodettoRectangle invalid = xsmcGetHostChunk(xsArg(0));
		(sd->dispatch->doAdaptInvalid)(sd, invalid);
	}
}

void xs_ILI9341_pixelsToBytes(xsMachine *the)
{
	int count = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, ((count * kCommodettoPixelSize) + 7) >> 3);
}

void xs_ILI9341_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_ILI9341_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_ILI9341_WIDTH);
}

void xs_ILI9341_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_ILI9341_HEIGHT);
}

void xs_ILI9341_get_clut(xsMachine *the)
{
#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	spiDisplay sd = xsmcGetHostData(xsThis);
	if (sd->clut) {
		xsResult = xsNewHostObject(NULL);
		xsmcSetHostData(xsResult, sd->clut);
	}
#else
	// returns undefined
#endif
}

void xs_ILI9341_set_clut(xsMachine *the)
{
#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	spiDisplay sd = xsmcGetHostData(xsThis);
	sd->clut = xsmcGetHostData(xsArg(0));		// cannot be array buffer
#else
	xsUnknownError("unsupported");
#endif
}

void xs_ILI9341_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
// caller provides little-endian pixels, convert to big-endian for display
void ili9341Send_16LE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxSwap16(&sd->spiConfig, (void *)pixels, byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB565BE
// caller provides big-endian pixels, transfer directly to display
void ili9341Send_16BE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITx(&sd->spiConfig, (void *)pixels, byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
// caller provides 8-bit gray pixels, convert for  display
void ili9341Send_Gray256(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxGray256To16BE(&sd->spiConfig, (void *)pixels, byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332
// caller provides 332 RGB pixels, convert for  display
void ili9341Send_RGB332(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxRGB332To16BE(&sd->spiConfig, (void *)pixels, byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray16
// caller provides 4-bit gray pixels, convert for  display
void ili9341Send_Gray16(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxGray16To16BE(&sd->spiConfig, (void *)pixels, byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
// caller provides 4-bit CLUT pixels, convert for  display
void ili9341Send_CLUT16(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxCLUT16To16BE(&sd->spiConfig, (void *)pixels, byteLength, (uint16_t *)(sd->clut + ((16 * 16 * 16) + (16 * 2))));
}
#endif

void ili9341Command(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count)
{
	modSPIFlush();
   	SCREEN_DC_COMMAND;
   	modSPITxRx(&sd->spiConfig, &command, 1);		// could use modSPITx, but modSPITxRx is synchronous and callers rely on that

    if (count) {
    	SCREEN_DC_DATA;
        modSPITxRx(&sd->spiConfig, (uint8_t *)data, count);
    }
}

void ili9341Init(spiDisplay sd)
{
	uint8_t data[15];

	SCREEN_CS_INIT;
	SCREEN_DC_INIT;
	modSPIInit(&sd->spiConfig);

#ifdef MODDEF_ILI9341_RST_PIN
	SCREEN_RST_INIT;
	SCREEN_RST_ACTIVE;
	modDelayMilliseconds(10);
	SCREEN_RST_DEACTIVE;
	modDelayMilliseconds(1);
#endif

	data[0] = 0x39;
	data[1] = 0x2C;
	data[2] = 0x00;
	data[3] = 0x34;
	data[4] = 0x02;
	ili9341Command(sd, 0xCB, data, 5);

	data[0] = 0x00;
	data[1] = 0XC1;
	data[2] = 0X30;
	ili9341Command(sd, 0xCF, data, 3);

	data[0] = 0x85;
	data[1] = 0x00;
	data[2] = 0x78;
	ili9341Command(sd, 0xE8, data, 3);

	data[0] = 0x00;
	data[1] = 0x00;
	ili9341Command(sd, 0xEA, data, 2);

	data[0] = 0x64;
	data[1] = 0x03;
	data[2] = 0X12;
	data[3] = 0X81;
	ili9341Command(sd, 0xED, data, 4);

	data[0] = 0x20;
	ili9341Command(sd, 0xF7, data, 1);

	data[0] = 0x23;   	//VRH[5:0]
	ili9341Command(sd, 0xC0, data, 1);    	//Power control

	data[0] = 0x10;   	//SAP[2:0];BT[3:0]
	ili9341Command(sd, 0xC1, data, 1);    	//Power control

	data[0] = 0x3e;   	//Contrast
	data[1] = 0x28;
	ili9341Command(sd, 0xC5, data, 2);    	//VCM control

	data[0] = 0x86;  	 //--
	ili9341Command(sd, 0xC7, data, 1);    	//VCM control2

	data[0] = 0x48;
	if (MODDEF_ILI9341_FLIPX)
		data[0] ^= 0x40;
	if (MODDEF_ILI9341_FLIPY)
		data[0] ^= 0x80;
	ili9341Command(sd, 0x36, data, 1);    	// Memory Access Control

	data[0] = 0x55;							// 16 bit
	ili9341Command(sd, 0x3A, data, 1);		// pixel format

	data[0] = 0x00;
	data[1] = 0x18;							// ?? Hz
	ili9341Command(sd, 0xB1, data, 2);		// frame rate

	data[0] = 0x08;
	data[1] = 0x82;
	data[2] = 0x27;
	ili9341Command(sd, 0xB6, data, 3);    	// Display Function Control

	data[0] = 0x00;
	ili9341Command(sd, 0xF2, data, 1);    	// 3Gamma Function Disable

	data[0] = 0x01;
	ili9341Command(sd, 0x26, data, 1);    	//Gamma curve selected

	data[0] = 0x0F;
	data[1] = 0x31;
	data[2] = 0x2B;
	data[3] = 0x0C;
	data[4] = 0x0E;
	data[5] = 0x08;
	data[6] = 0x4E;
	data[7] = 0xF1;
	data[8] = 0x37;
	data[9] = 0x07;
	data[10] = 0x10;
	data[11] = 0x03;
	data[12] = 0x0E;
	data[13] = 0x09;
	data[14] = 0x00;
	ili9341Command(sd, 0xE0, data, 15);    	// Set positive gamma

	data[0] = 0x00;
	data[1] = 0x0E;
	data[2] = 0x14;
	data[3] = 0x03;
	data[4] = 0x11;
	data[5] = 0x07;
	data[6] = 0x31;
	data[7] = 0xC1;
	data[8] = 0x48;
	data[9] = 0x08;
	data[10] = 0x0F;
	data[11] = 0x0C;
	data[12] = 0x31;
	data[13] = 0x36;
	data[14] = 0x0F;
	ili9341Command(sd, 0xE1, data, 15);    	// Set negative gamma

	ili9341Command(sd, 0x11, 0, 0);    	// Exit Sleep
//@@	modDelayMilliseconds(120);		// delay in some sample code. but doesn't seem to be required (check data sheet)

	ili9341Command(sd, 0x29, 0, 0);    // Display on
	ili9341Command(sd, 0x2c, 0, 0);
}

void ili9341ChipSelect(uint8_t active, modSPIConfiguration config)
{
	spiDisplay sd = (spiDisplay)(((char *)config) - offsetof(spiDisplayRecord, spiConfig));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}

void ili9341Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint8_t data[4];
	uint16_t xMin, xMax, yMin, yMax;

	xMin = x;
	yMin = y;
	xMax = xMin + w - 1;
	yMax = yMin + h - 1;

	data[0] = xMin >> 8;
	data[1] = xMin & 0xff;
	data[2] = xMax >> 8;
	data[3] = xMax & 0xff;
	ili9341Command(sd, 0x2a, data, sizeof(data));

	data[0] = yMin >> 8;
	data[1] = yMin & 0xff;
	data[2] = yMax >> 8;
	data[3] = yMax & 0xff;
	ili9341Command(sd, 0x2b, data, sizeof(data));

    ili9341Command(sd, 0x2c, 0, 0);

	SCREEN_DC_DATA;
}

void ili9341End(void *refcon)
{
}

void ili9341AdaptInvalid(void *refcon, CommodettoRectangle invalid)
{
	spiDisplay sd = refcon;

	if (invalid->x & 1) {
		invalid->x -= 1;
		invalid->w += 1;
	}
	if (invalid->w & 1) {
		invalid->w += 1;
	}

	// left clipped
	if (invalid->x < 0) {
		if (-invalid->x >= invalid->w) {
			invalid->w = 0;
			return;
		}
		invalid->w += invalid->x;
		invalid->x = 0;
	}

	// right clipped
	if ((invalid->x + invalid->w) > MODDEF_ILI9341_WIDTH) {
		if (MODDEF_ILI9341_WIDTH <= invalid->x) {
			invalid->w = 0;
			return;
		}
		invalid->w = MODDEF_ILI9341_WIDTH - invalid->x;
	}
}
