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

// modeled on Adafruit SSD1351 library.
//  https://github.com/adafruit/Adafruit-SSD1351-library
#include "xsmc.h"
#include "xsHost.h"
#include "modGPIO.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "stddef.h"		// for offsetof macro
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "modSPI.h"

/*
	ssd1351 defines from AdaFruit_SSD1351.h
*/

// Timing Delays
#define SSD1351_DELAYS_HWFILL	    (3)
#define SSD1351_DELAYS_HWLINE       (1)

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN 		0x15
#define SSD1351_CMD_SETROW    		0x75
#define SSD1351_CMD_WRITERAM   		0x5C
#define SSD1351_CMD_READRAM   		0x5D
#define SSD1351_CMD_SETREMAP 		0xA0
#define SSD1351_CMD_STARTLINE 		0xA1
#define SSD1351_CMD_DISPLAYOFFSET 	0xA2
#define SSD1351_CMD_DISPLAYALLOFF 	0xA4
#define SSD1351_CMD_DISPLAYALLON  	0xA5
#define SSD1351_CMD_NORMALDISPLAY 	0xA6
#define SSD1351_CMD_INVERTDISPLAY 	0xA7
#define SSD1351_CMD_FUNCTIONSELECT 	0xAB
#define SSD1351_CMD_DISPLAYOFF 		0xAE
#define SSD1351_CMD_DISPLAYON     	0xAF
#define SSD1351_CMD_PRECHARGE 		0xB1
#define SSD1351_CMD_DISPLAYENHANCE	0xB2
#define SSD1351_CMD_CLOCKDIV 		0xB3
#define SSD1351_CMD_SETVSL 			0xB4
#define SSD1351_CMD_SETGPIO 		0xB5
#define SSD1351_CMD_PRECHARGE2 		0xB6
#define SSD1351_CMD_SETGRAY 		0xB8
#define SSD1351_CMD_USELUT 			0xB9
#define SSD1351_CMD_PRECHARGELEVEL 	0xBB
#define SSD1351_CMD_VCOMH 			0xBE
#define SSD1351_CMD_CONTRASTABC		0xC1
#define SSD1351_CMD_CONTRASTMASTER	0xC7
#define SSD1351_CMD_MUXRATIO		0xCA
#define SSD1351_CMD_COMMANDLOCK		0xFD
#define SSD1351_CMD_HORIZSCROLL		0x96
#define SSD1351_CMD_STOPSCROLL		0x9E
#define SSD1351_CMD_STARTSCROLL		0x9F

#define SSD1351_CMD_NULL			0x00

#ifndef MODDEF_SSD1351_CS_PORT
	#define MODDEF_SSD1351_CS_PORT NULL
#endif
#ifndef MODDEF_SSD1351_RST_PORT
	#define MODDEF_SSD1351_RST_PORT NULL
#endif
#ifndef MODDEF_SSD1351_DC_PORT
	#define MODDEF_SSD1351_DC_PORT NULL
#endif
#ifndef MODDEF_SSD1351_OFFSET_COLUMN
	#define MODDEF_SSD1351_OFFSET_COLUMN (0)
#endif
#ifndef MODDEF_SSD1351_OFFSET_ROW
	#define MODDEF_SSD1351_OFFSET_ROW (0)
#endif
#ifndef MODDEF_SSD1351_HZ
	#define MODDEF_SSD1351_HZ (10000000)
#endif

#ifndef MODDEF_SSD1351_INITIALIZATION
/* Default init sequence: 128x128 */
static const uint8_t gSSD1351Initialization[] ICACHE_FLASH_ATTR = {
	0xFD, 1, 0x12,
	0xFD, 1, 0xB1,
	0xAE, 0,
	0xB3, 1, 0xF1,
	0xCA, 1, 0x7F,
	0xA0, 1, 0x74,
	0x15, 2, 0x00, 0x7F,
	0x75, 2, 0x00, 0x7F,
	0xA1, 1, 0,						// patched below based on height
	0xA2, 1, 0x00,
	0xB5, 1, 0x00,
	0xAB, 1, 0x01,
	0xB1, 1, 0x32,
	0xBE, 1, 0x05,
	0xA6, 0,
	0xC1, 3, 0xC8, 0x80, 0xC8,
	0xC7, 1, 0x0F,
	0xB4, 3, 0xA0, 0xB5, 0x55,
	0xB6, 1, 0x01,
	0xAF, 0,
	0x00
};
#else
static const uint8_t gSSD1351Initialization[] ICACHE_FLASH_ATTR = MODDEF_SSD1351_INITIALIZATION;
#endif

#ifdef MODDEF_SSD1351_CS_PIN
	#define SCREEN_CS_ACTIVE		modGPIOWrite(&sd->cs, 0)
	#define SCREEN_CS_DEACTIVE		modGPIOWrite(&sd->cs, 1)
	#define SCREEN_CS_INIT			modGPIOInit(&sd->cs, MODDEF_SSD1351_CS_PORT, MODDEF_SSD1351_CS_PIN, kModGPIOOutput); \
									SCREEN_CS_DEACTIVE
#else
	#define SCREEN_CS_ACTIVE
	#define SCREEN_CS_DEACTIVE
	#define SCREEN_CS_INIT
#endif

#define SCREEN_DC_DATA			modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&sd->dc, MODDEF_SSD1351_DC_PORT, MODDEF_SSD1351_DC_PIN, kModGPIOOutput); \
								SCREEN_DC_DATA

#ifdef MODDEF_SSD1351_RST_PIN
	#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
	#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
	#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, MODDEF_SSD1351_RST_PORT, MODDEF_SSD1351_RST_PIN, kModGPIOOutput); \
									SCREEN_RST_DEACTIVE
#else
	#define SCREEN_RST_ACTIVE
	#define SCREEN_RST_DEACTIVE
	#define SCREEN_RST_INIT
#endif

typedef struct {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;

#ifdef MODDEF_SSD1351_CS_PIN
	modGPIOConfigurationRecord	cs;
#endif
	modGPIOConfigurationRecord	dc;
#ifdef MODDEF_SSD1351_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif
} spiDisplayRecord, *spiDisplay;

static void ssd1351ChipSelect(uint8_t active, modSPIConfiguration config);

static void ssd1351Init(spiDisplay sd);
static void ssd1351Command(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count);

static void ssd1351Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void ssd1351Continue(void *refcon);
static void ssd1351End(void *refcon);
static void ssd1351Send(void *refcon, PocoPixel *pixels, uint32_t byteLength);

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
static void ssd1351Send_16LE(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch_16BE ICACHE_RODATA_ATTR = {
	ssd1351Begin,
	ssd1351Continue,
	ssd1351End,
	ssd1351Send_16LE,
	NULL
};
#else
	#error rgb565le pixels required
#endif

void xs_ssd1351_destructor(void *data)
{
	if (data) {
		spiDisplay sd = (spiDisplay)data;
		modSPIUninit(&sd->spiConfig);
		c_free(data);
	}
}

void xs_ssd1351(xsMachine *the)
{
	spiDisplay sd;

	sd = c_calloc(1, sizeof(spiDisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

	modSPIConfig(sd->spiConfig, MODDEF_SSD1351_HZ, MODDEF_SSD1351_SPI_PORT,
			MODDEF_SSD1351_CS_PORT, MODDEF_SSD1351_CS_PIN, ssd1351ChipSelect);

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch_16BE;

	ssd1351Init(sd);
}

void xs_ssd1351_begin(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	ssd1351Begin(sd, x, y, w, h);
}

void xs_ssd1351_send(xsMachine *the)
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

void xs_ssd1351_end(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	ssd1351End(sd);
}

void xs_ssd1351_adaptInvalid(xsMachine *the)
{
}

void xs_ssd1351_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_ssd1351_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_SSD1351_WIDTH);
}

void xs_ssd1351_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_SSD1351_HEIGHT);
}

void xs_ssd1351_get_clut(xsMachine *the)
{
	// returns undefined
}

void xs_ssd1351_set_clut(xsMachine *the)
{
	xsUnknownError("unsupported");
}

void xs_ssd1351_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
// caller provides little-endian pixels, convert to big-endian for display
void ssd1351Send_16LE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	modSPITxSwap16(&sd->spiConfig, (void *)pixels, (byteLength < 0) ? -byteLength : byteLength);
}
#endif

void ssd1351Command(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count)
{
	modSPIFlush();
	SCREEN_DC_COMMAND;
	modSPITxRx(&sd->spiConfig, &command, sizeof(command));		// could use modSPITx, but modSPITxRx is synchronous and callers rely on that

	if (count) {
		SCREEN_DC_DATA;
		modSPITxRx(&sd->spiConfig, (uint8_t *)data, count);
	}
}

void ssd1351Init(spiDisplay sd)
{
	uint8_t i = 0;

	SCREEN_CS_INIT;
	SCREEN_DC_INIT;
	SCREEN_RST_INIT;

	modSPIInit(&sd->spiConfig);

	// N.B. Adafruit sample uses 500 millisecond delay... seems _very_ long... 500 microsceconds works just fine
	SCREEN_CS_ACTIVE;
	SCREEN_RST_DEACTIVE;
	modDelayMicroseconds(500);
	SCREEN_RST_ACTIVE;
	modDelayMicroseconds(500);
	SCREEN_RST_DEACTIVE;
	modDelayMicroseconds(500);

	while (i < sizeof (gSSD1351Initialization)) {
		uint8_t command, count;
		uint8_t data[16];

		command = c_read8(&gSSD1351Initialization[i++]);
		if (SSD1351_CMD_NULL == command)
			break;

		count = c_read8(&gSSD1351Initialization[i++]);
		if (count > 0) {
			c_memcpy(data, gSSD1351Initialization + i, count);
			i += count;

			if (SSD1351_CMD_STARTLINE == command)
				data[0] = (96 == MODDEF_SSD1351_HEIGHT) ? 96 : 0;
		}

		ssd1351Command(sd, command, data, count);
	}
}

void ssd1351ChipSelect(uint8_t active, modSPIConfiguration config)
{
	spiDisplay sd = (spiDisplay)(((char *)config) - offsetof(spiDisplayRecord, spiConfig));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}

void ssd1351Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint8_t data[2];
	uint16_t xMin, xMax, yMin, yMax;

	xMin = x;
	yMin = y;
	xMax = xMin + w - 1;
	yMax = yMin + h - 1;

	data[0] = xMin + MODDEF_SSD1351_OFFSET_COLUMN;
	data[1] = xMax + MODDEF_SSD1351_OFFSET_COLUMN;
	ssd1351Command(sd, SSD1351_CMD_SETCOLUMN, data, sizeof(data));

	data[0] = yMin + MODDEF_SSD1351_OFFSET_ROW;
	data[1] = yMax + MODDEF_SSD1351_OFFSET_ROW;
	ssd1351Command(sd, SSD1351_CMD_SETROW, data, sizeof(data));

	ssd1351Command(sd, SSD1351_CMD_WRITERAM, NULL, 0);

	SCREEN_DC_DATA;
}

void ssd1351Continue(void *refcon)
{
	ssd1351End(refcon);
}

void ssd1351End(void *refcon)
{
}
