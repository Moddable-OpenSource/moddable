/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "xsHost.h"

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
#ifndef MODDEF_ILI9341_BACKLIGHT_PORT
	#define MODDEF_ILI9341_BACKLIGHT_PORT NULL
#endif
#ifndef MODDEF_ILI9341_BACKLIGHT_ON
	#define MODDEF_ILI9341_BACKLIGHT_ON (0)
#endif
#if MODDEF_ILI9341_BACKLIGHT_ON
	#define MODDEF_ILI9341_BACKLIGHT_OFF (0)
#else
	#define MODDEF_ILI9341_BACKLIGHT_OFF (1)
#endif
#ifndef MODDEF_ILI9341_SPI_PORT
	#define MODDEF_ILI9341_SPI_PORT NULL
#endif
#ifndef MODDEF_ILI9341_COLUMN_OFFSET
	#define MODDEF_ILI9341_COLUMN_OFFSET 0
#endif
#ifndef MODDEF_ILI9341_ROW_OFFSET
	#define MODDEF_ILI9341_ROW_OFFSET 0
#endif

#ifdef MODDEF_ILI9341_CS_PIN
	#define SCREEN_CS_ACTIVE	modGPIOWrite(&sd->cs, 0)
	#define SCREEN_CS_DEACTIVE	modGPIOWrite(&sd->cs, 1)
	#define SCREEN_CS_INIT		modGPIOInit(&sd->cs, MODDEF_ILI9341_CS_PORT, MODDEF_ILI9341_CS_PIN, kModGPIOOutput); \
			SCREEN_CS_DEACTIVE
	#define SCREEN_CS_UNINIT	modGPIOUninit(&sd->cs);
#else
	#define MODDEF_ILI9341_CS_PIN	NULL
	#define SCREEN_CS_ACTIVE
	#define SCREEN_CS_DEACTIVE
	#define SCREEN_CS_INIT
	#define SCREEN_CS_UNINIT
#endif

#ifndef MODDEF_ILI9341_SPI_MODE
       #define MODDEF_ILI9341_SPI_MODE 0
#endif

#define SCREEN_DC_DATA			modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&sd->dc, MODDEF_ILI9341_DC_PORT, MODDEF_ILI9341_DC_PIN, kModGPIOOutput); \
		SCREEN_DC_DATA
#define SCREEN_DC_UNINIT		modGPIOUninit(&sd->dc)
#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, MODDEF_ILI9341_RST_PORT, MODDEF_ILI9341_RST_PIN, kModGPIOOutput); \
		SCREEN_RST_DEACTIVE;
#define SCREEN_RST_UNINIT		modGPIOUninit(&sd->rst);

#ifndef ILI9341_GRAM_WIDTH
	#define ILI9341_GRAM_WIDTH		MODDEF_ILI9341_WIDTH
#endif
#ifndef ILI9341_GRAM_HEIGHT
	#define ILI9341_GRAM_HEIGHT		MODDEF_ILI9341_HEIGHT
#endif

#define ILI9341_GRAM_X_OFFSET  ILI9341_GRAM_WIDTH - MODDEF_ILI9341_WIDTH
#define ILI9341_GRAM_Y_OFFSET  ILI9341_GRAM_HEIGHT - MODDEF_ILI9341_HEIGHT

typedef struct {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;

#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	uint16_t					clut[16];
	uint8_t						haveCLUT;
#endif

	modGPIOConfigurationRecord	cs;
	modGPIOConfigurationRecord	dc;
#ifdef MODDEF_ILI9341_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif
#ifdef MODDEF_ILI9341_BACKLIGHT_PIN
	modGPIOConfigurationRecord	backlight;
#endif
	uint8_t						firstFrame;
	uint8_t						memoryAccessControl;	// register 36h initialization value
	uint8_t						rotation;				// 0, 1, 2, 3 => 0, 90, 180, 270
} spiDisplayRecord, *spiDisplay;

static void ili9341ChipSelect(uint8_t active, modSPIConfiguration config);

/* static */ void ili9341Init(spiDisplay sd);
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
	spiDisplay sd = data;
	if (!data) return;

	SCREEN_CS_UNINIT;
	SCREEN_DC_UNINIT;
	modSPIUninit(&sd->spiConfig);

#ifdef MODDEF_ILI9341_RST_PIN
	SCREEN_RST_UNINIT;
#endif
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
			MODDEF_ILI9341_CS_PORT, -1, ili9341ChipSelect);
	sd->spiConfig.mode = MODDEF_ILI9341_SPI_MODE;

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;

	SCREEN_CS_INIT;
	SCREEN_DC_INIT;
	modSPIInit(&sd->spiConfig);

#ifdef MODDEF_ILI9341_RST_PIN
	SCREEN_RST_INIT;
#endif

	ili9341Init(sd);

#if defined(MODDEF_ILI9341_ERASE_WIDTH) && defined(MODDEF_ILI9341_ERASE_HEIGHT)
	int i;
	uint8_t *pixels = c_calloc(MODDEF_ILI9341_ERASE_WIDTH, 2);
	if (pixels) {
		ili9341Begin(sd, 0, 0, MODDEF_ILI9341_ERASE_WIDTH, MODDEF_ILI9341_ERASE_HEIGHT);
		for (i = 0; i < MODDEF_ILI9341_ERASE_HEIGHT; i++)
			(sd->dispatch->doSend)((PocoPixel *)pixels, MODDEF_ILI9341_ERASE_WIDTH * 2, sd);
		ili9341End(sd);
		c_free(pixels);
	}
#endif

#ifdef MODDEF_ILI9341_BACKLIGHT_PIN
	modGPIOInit(&sd->backlight, MODDEF_ILI9341_BACKLIGHT_PORT, MODDEF_ILI9341_BACKLIGHT_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->backlight, MODDEF_ILI9341_BACKLIGHT_OFF);
#endif
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
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &count);

	if (argc > 1) {
		xsIntegerValue offset = xsmcToInteger(xsArg(1));

		if ((xsUnsignedValue)offset >= count)
			xsUnknownError("bad offset");
		data += offset;
		count -= offset;
		if (argc > 2) {
			xsIntegerValue c = xsmcToInteger(xsArg(2));
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
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, (sd->rotation & 1) ? MODDEF_ILI9341_HEIGHT : MODDEF_ILI9341_WIDTH);
}

void xs_ILI9341_get_height(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, (sd->rotation & 1) ? MODDEF_ILI9341_WIDTH : MODDEF_ILI9341_HEIGHT);
}

void xs_ILI9341_get_clut(xsMachine *the)
{
#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	spiDisplay sd = xsmcGetHostData(xsThis);
	if (sd->haveCLUT) {
		xsResult = xsNewHostObject(NULL);
		xsmcSetHostBuffer(xsResult, sd->clut, sizeof(sd->clut));
	}
#else
	// returns undefined
#endif
}

void xs_ILI9341_set_clut(xsMachine *the)
{
#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	spiDisplay sd = xsmcGetHostData(xsThis);
	void *data;
	xsUnsignedValue dataSize;

	xsmcGetBufferReadable(xsArg(0), &data, &dataSize);
	if (dataSize > sizeof(sd->clut))
		xsUnknownError("invalid");

	c_memmove(sd->clut, data, dataSize);
	sd->haveCLUT = 1;
#else
	xsUnknownError("unsupported");
#endif
}

void xs_ILI9341_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void xs_ILI9341_get_rotation(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, sd->rotation * 90);
}

void xs_ILI9341_set_rotation(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	int32_t rotation = xsmcToInteger(xsArg(0));
	uint8_t value;
	static const uint8_t masks[] ICACHE_RODATA_ATTR = {0x00, 0x60, 0xc0, 0xa0};
	if ((0 != rotation) && (90 != rotation) && (180 != rotation) && (270 != rotation))
		xsRangeError("invalid rotation");

	sd->rotation = (uint8_t)(rotation / 90);
	value = sd->memoryAccessControl ^ c_read8(masks + sd->rotation);
	ili9341Command(sd, 0x36, &value, 1);
}

void xs_ILI9341_command(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	uint8_t command = (uint8_t)xsmcToInteger(xsArg(0));
	uint16_t dataSize = 0;
	uint8_t *data = NULL;

	if (xsmcArgc > 1) {
		dataSize = (uint16_t)xsmcGetArrayBufferLength(xsArg(1));
		data = xsmcToArrayBuffer(xsArg(1));
	}

	ili9341Command(sd, command, data, dataSize);
}

void xs_ILI9341_close(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	if (!sd) return;
	xs_ILI9341_destructor(sd);
	xsmcSetHostData(xsThis, NULL);
}

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
// caller provides little-endian pixels, convert to big-endian for display
void ili9341Send_16LE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITxSwap16(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB565BE
// caller provides big-endian pixels, transfer directly to display
void ili9341Send_16BE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITx(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
// caller provides 8-bit gray pixels, convert for  display
void ili9341Send_Gray256(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITxGray256To16BE(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332
// caller provides 332 RGB pixels, convert for  display
void ili9341Send_RGB332(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITxRGB332To16BE(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray16
// caller provides 4-bit gray pixels, convert for  display
void ili9341Send_Gray16(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITxGray16To16BE(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
// caller provides 4-bit CLUT pixels, convert for  display
void ili9341Send_CLUT16(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPISetSync(&sd->spiConfig, byteLength > 0);
	modSPITxCLUT16To16BE(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength, sd->clut);
}
#endif

void ili9341Command(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count)
{
	modSPIActivateConfiguration(NULL);
	SCREEN_DC_COMMAND;
   	modSPITxRx(&sd->spiConfig, &command, 1);		// could use modSPITx, but modSPITxRx is synchronous and callers rely on that

	if (count) {
        SCREEN_DC_DATA;
        modSPITxRx(&sd->spiConfig, (uint8_t *)data, count);
    }
}

// delay of 0 is end of commands
#define kDelayMS (255)

#define kILI9341RegistersModdableZero_Start \
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02, \
	0xCF, 3, 0x00, 0xC1, 0X30, \
	0xE8, 3, 0x85, 0x00, 0x78, \
	0xEA, 2, 0x00, 0x00, \
	0xED, 4, 0x64, 0x03, 0x12, 0x81, \
	0xF7, 1, 0x20, \
	0xC0, 1, 0x23, \
	0xC1, 1, 0x10, \
	0xC5, 2, 0x3e, 0x28, \
	0xC7, 1, 0x86, \
	0x36, 1, (0x48 ^ (MODDEF_ILI9341_FLIPY ? 0x80 : 0)) ^ (MODDEF_ILI9341_FLIPX ? 0x40 : 0), \
	0x3A, 1, 0x55, \
	0xB1, 2, 0x00, 0x18, \
	0xB6, 3, 0x08, 0x82, 0x27, \
	0xF2, 1, 0x00, \
	0x26, 1, 0x01, \
	0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, \
	0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,

#define kILI9341RegistersModdableZero_Finish \
	kDelayMS, 0


static const uint8_t gInit[] ICACHE_RODATA_ATTR = {
#ifdef MODDEF_ILI9341_REGISTERS_APPEND
	kILI9341RegistersModdableZero_Start
	MODDEF_ILI9341_REGISTERS_APPEND
	kILI9341RegistersModdableZero_Finish
#elif defined(MODDEF_ILI9341_REGISTERS)
	MODDEF_ILI9341_REGISTERS
#else
	kILI9341RegistersModdableZero_Start
	kILI9341RegistersModdableZero_Finish
#endif
};

void ili9341Init(spiDisplay sd)
{
	uint8_t data[16] __attribute__((aligned(4)));
	const uint8_t *cmds;

#ifdef MODDEF_ILI9341_RST_PIN
	SCREEN_RST_ACTIVE;
	modDelayMilliseconds(10);
	SCREEN_RST_DEACTIVE;
	modDelayMilliseconds(1);
#endif

	cmds = gInit;
	while (true) {
		uint8_t cmd = c_read8(cmds++);
		if (kDelayMS == cmd) {
			uint8_t ms = c_read8(cmds++);
			if (0 == ms)
				break;
			modDelayMilliseconds(ms);
		}
		else {
			if (0x36 == cmd)
				sd->memoryAccessControl = c_read8(cmds + 1);
			uint8_t count = c_read8(cmds++);
			if (count)
				c_memcpy(data, cmds, count);
			ili9341Command(sd, cmd, data, count);
			cmds += count;
		}
	}

	sd->firstFrame = true;
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

	xMin = x + MODDEF_ILI9341_COLUMN_OFFSET;
	yMin = y + MODDEF_ILI9341_ROW_OFFSET;
	
	switch (sd->rotation) {
		case 1:
			// 90 degrees
			yMin += ILI9341_GRAM_X_OFFSET;
			break;
		case 2:
			// 180 degrees
			yMin += ILI9341_GRAM_Y_OFFSET;
			xMin += ILI9341_GRAM_X_OFFSET;
			break;
		case 3:
			// 270 degrees
			xMin += ILI9341_GRAM_Y_OFFSET;
			break;
	}
	
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

    ili9341Command(sd, 0x2c, NULL, 0);

	SCREEN_DC_DATA;
}

void ili9341End(void *refcon)
{
	spiDisplay sd = refcon;

	if (sd->firstFrame) {
		sd->firstFrame = false;

		ili9341Command(sd, 0x11, NULL, 0);
		ili9341Command(sd, 0x29, NULL, 0);

#ifdef MODDEF_ILI9341_BACKLIGHT_PIN
		modGPIOWrite(&sd->backlight, MODDEF_ILI9341_BACKLIGHT_ON);
#endif
	}
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
