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
#include "xsHost.h"
#include "mc.defines.h"

#if MODDEF_SSD1306_SPI
	#include "modSPI.h"
#endif
#include "modGPIO.h"
#if MODDEF_SSD1306_I2C
	#include "modI2C.h"
 #endif

#include "commodettoPixelsOut.h"

#include "mc.xs.h"			// for xsID_ values

#if !MODDEF_SSD1306_SPI && !MODDEF_SSD1306_I2C
	#error "Must select set either MODDEF_SSD1306_SPI or MODDEF_SSD1306_I2C"
#endif
#if MODDEF_SSD1306_SPI && MODDEF_SSD1306_I2C
	#error "Must select set ony MODDEF_SSD1306_SPI or MODDEF_SSD1306_I2C"
#endif
#ifndef MODDEF_SSD1306_CS_PORT
	#define MODDEF_SSD1306_CS_PORT NULL
#endif
#ifndef MODDEF_SSD1306_RST_PORT
	#define MODDEF_SSD1306_RST_PORT NULL
#endif
#ifndef MODDEF_SSD1306_DC_PORT
	#define MODDEF_SSD1306_DC_PORT NULL
#endif
#ifndef MODDEF_SSD1306_DITHER
	#define MODDEF_SSD1306_DITHER (0)
#endif
#ifndef MODDEF_SSD1306_HZ
	#if MODDEF_SSD1306_SPI
		#define MODDEF_SSD1306_HZ (10000000)
	#elif MODDEF_SSD1306_I2C
		#define MODDEF_SSD1306_HZ (600000)
	#endif
#endif
#ifndef MODDEF_SSD1306_ADDRESS
	#define MODDEF_SSD1306_ADDRESS (0x3C)
#endif

/*
	crazy pixel layout
	
	8 scan horizonatal scan lines per page
	(so 128 bytes is a page)
	each byte contains 8 VERTICAL pixels
*/


#define SCREEN_CS_ACTIVE		modGPIOWrite(&ssd->cs, 0)
#define SCREEN_CS_DEACTIVE		modGPIOWrite(&ssd->cs, 1)
#define SCREEN_CS_INIT			modGPIOInit(&ssd->cs, MODDEF_SSD1306_CS_PORT, MODDEF_SSD1306_CS_PIN, kModGPIOOutput); \
								SCREEN_CS_DEACTIVE

#define SCREEN_DC_DATA			modGPIOWrite(&ssd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&ssd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&ssd->dc, MODDEF_SSD1306_DC_PORT, MODDEF_SSD1306_DC_PIN, kModGPIOOutput); \
								SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		modGPIOWrite(&ssd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&ssd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&ssd->rst, MODDEF_SSD1306_RST_PORT, MODDEF_SSD1306_RST_PIN, kModGPIOOutput); \
								SCREEN_RST_DEACTIVE;

#define SSD1306_MAXWIDTH (128)

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define vccstate SSD1306_SWITCHCAPVCC

#define kBufferSlop (4)

typedef union configSPIandI2C {
#if MODDEF_SSD1306_SPI
	modSPIConfigurationRecord	spi;
#elif MODDEF_SSD1306_I2C
	modI2CConfigurationRecord	i2c;
#endif
} configSPIandI2C;

struct ssd1606Record {
	PixelsOutDispatch	dispatch;
	xsMachine			*the;

	configSPIandI2C		config;

#if MODDEF_SSD1306_SPI
	modGPIOConfigurationRecord	cs;
	modGPIOConfigurationRecord	dc;
#endif

#ifdef MODDEF_SSD1306_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif

	uint8_t				pixel;										// mask for white pixel on current row

	uint8_t				width;
	uint8_t				height;

	uint32_t			pad;										// ensure buffer is 4-byte aligned (SPI code requires that)
	uint8_t				buffer[kBufferSlop + SSD1306_MAXWIDTH];		// 8 rows of 1-bit pixels, plus 32-bit header to allow I2C transmission in-place

#if MODDEF_SSD1306_DITHER
	uint8_t				ditherPhase;
	int16_t				ditherA[SSD1306_MAXWIDTH + 4];
	int16_t				ditherB[SSD1306_MAXWIDTH + 4];
#endif
};
typedef struct ssd1606Record ssd1606Record;
typedef ssd1606Record *ssd1606;

static void doCmd(ssd1606 ssd, uint8_t cmd);
#if MODDEF_SSD1306_SPI
	static void ssd1306ChipSelect(uint8_t active, modSPIConfiguration config);
#endif

static void ssd1306Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void ssd1306Continue(void *refcon);
static void ssd1306End(void *refcon);
static void ssd1306Send(PocoPixel *pixels, int byteLength, void *refcon);
static void ssd1306AdaptInvalid(void *refcon, CommodettoRectangle r);

typedef void (*PocoRenderedPixelsReceiver)(PocoPixel *pixels, int byteCount, void *refCon);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ssd1306Begin,
	ssd1306Continue,
	ssd1306End,
	ssd1306Send,
	ssd1306AdaptInvalid
};

void xs_SSD1306_destructor(void *data)
{
	if (data) {
		ssd1606 ssd = (ssd1606)data;
#if MODDEF_SSD1306_SPI
		modSPIUninit(&ssd->config.spi);
#elif MODDEF_SSD1306_I2C
		modI2CUninit(&ssd->config.i2c);
#endif
		c_free(data);
	}
}

void xs_SSD1306(xsMachine *the)
{
	int width, height, pixelFormat;
	int address;
	ssd1606 ssd;

	xsmcVars(1);
#if MODDEF_SSD1306_WIDTH
	width = MODDEF_SSD1306_WIDTH;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_width);
	width = xsmcToInteger(xsVar(0));
#endif

#if MODDEF_SSD1306_HEIGHT
	height = MODDEF_SSD1306_HEIGHT;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_height);
	height = xsmcToInteger(xsVar(0));
#endif

	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		pixelFormat = xsmcToInteger(xsVar(0));
	}
	else
		pixelFormat = kCommodettoBitmapFormat;
	if (kCommodettoBitmapGray256 != pixelFormat)
		xsUnknownError("gray256 pixels required");

	ssd = c_calloc(1, sizeof(ssd1606Record));
	if (!ssd)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, ssd);

	ssd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;
	ssd->the = the;

	ssd->width = (uint8_t)width;
	ssd->height = (uint8_t)height;
	
#ifdef MODDEF_SSD1306_RST_PIN
	SCREEN_RST_INIT;
	// modDelayMilliseconds(10);
	SCREEN_RST_DEACTIVE;
	modDelayMilliseconds(1);
	SCREEN_RST_ACTIVE;
	modDelayMilliseconds(10);
	SCREEN_RST_DEACTIVE;
#endif

#if MODDEF_SSD1306_SPI
	SCREEN_CS_INIT;
	SCREEN_DC_INIT;

	ssd->config.spi.hz = MODDEF_SSD1306_HZ;
	ssd->config.spi.doChipSelect = ssd1306ChipSelect;
	modSPIInit(&ssd->config.spi);

#elif MODDEF_SSD1306_I2C
	#ifdef MODDEF_SSD1306_SDA_PIN
		ssd->config.i2c.sda = MODDEF_SSD1306_SDA_PIN;
	#else
		xsmcGet(xsVar(0), xsArg(0), xsID_sda);
		ssd->config.i2c.sda = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	#endif
	#ifdef MODDEF_SSD1306_SCL_PIN
		ssd->config.i2c.scl = MODDEF_SSD1306_SCL_PIN;
	#else
		xsmcGet(xsVar(0), xsArg(0), xsID_scl);
		ssd->config.i2c.scl = (xsUndefinedType == xsmcTypeOf(xsVar(0))) ? -1 : xsmcToInteger(xsVar(0));
	#endif

#ifdef MODDEF_SSD1306_ADDRESS
	ssd->config.i2c.address = MODDEF_SSD1306_ADDRESS;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	ssd->config.i2c.address = (uint8_t)xsmcToInteger(xsVar(0);
#endif
	ssd->config.i2c.hz = MODDEF_SSD1306_HZ;
	modI2CInit(&ssd->config.i2c);

	ssd->buffer[kBufferSlop - 1] = 0x40;
#endif

	// Init sequence
	doCmd(ssd, SSD1306_DISPLAYOFF);                    // 0xAE
	doCmd(ssd, SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
	doCmd(ssd, 0x80);                                  // the suggested ratio 0x80

	doCmd(ssd, SSD1306_SETMULTIPLEX);                  // 0xA8
	doCmd(ssd, ssd->height - 1);

	doCmd(ssd, SSD1306_SETDISPLAYOFFSET);              // 0xD3
	doCmd(ssd, 0x0);                                   // no offset
	doCmd(ssd, SSD1306_SETSTARTLINE | 0x0);            // line #0
	doCmd(ssd, SSD1306_CHARGEPUMP);                    // 0x8D
	if (vccstate == SSD1306_EXTERNALVCC)
	{ doCmd(ssd, 0x10); }
	else
	{ doCmd(ssd, 0x14); }
	doCmd(ssd, SSD1306_MEMORYMODE);                    // 0x20
	doCmd(ssd, 0x00);                                  // 0x0 act like ks0108
	doCmd(ssd, SSD1306_SEGREMAP | 0x1);
	doCmd(ssd, SSD1306_COMSCANDEC);

	if ((128 == ssd->width) && (32 == ssd->height)) {
		doCmd(ssd, SSD1306_SETCOMPINS);                    // 0xDA
		doCmd(ssd, 0x02);
		doCmd(ssd, SSD1306_SETCONTRAST);                   // 0x81
		doCmd(ssd, 0x8F);
	}
	else if ((128 == ssd->width) && (64 == ssd->height)) {
		doCmd(ssd, SSD1306_SETCOMPINS);                    // 0xDA
		doCmd(ssd, 0x12);
		doCmd(ssd, SSD1306_SETCONTRAST);                   // 0x81
		if (vccstate == SSD1306_EXTERNALVCC)
		{ doCmd(ssd, 0x9F); }
		else
		{ doCmd(ssd, 0xCF); }
	}
	else if ((96 == ssd->width) && (16 == ssd->height)) {
		doCmd(ssd, SSD1306_SETCOMPINS);                    // 0xDA
		doCmd(ssd, 0x2);   //ada x12
		doCmd(ssd, SSD1306_SETCONTRAST);                   // 0x81
		if (vccstate == SSD1306_EXTERNALVCC)
		{ doCmd(ssd, 0x10); }
		else
		{ doCmd(ssd, 0xAF); }
	}

	doCmd(ssd, SSD1306_SETPRECHARGE);                  // 0xd9
	if (vccstate == SSD1306_EXTERNALVCC)
	{ doCmd(ssd, 0x22); }
	else
	{ doCmd(ssd, 0xF1); }
	doCmd(ssd, SSD1306_SETVCOMDETECT);                 // 0xDB
	doCmd(ssd, 0x40);
	doCmd(ssd, SSD1306_DISPLAYALLON_RESUME);           // 0xA4
	doCmd(ssd, SSD1306_NORMALDISPLAY);                 // 0xA6

	doCmd(ssd, SSD1306_DEACTIVATE_SCROLL);

	doCmd(ssd, SSD1306_DISPLAYON);						//--turn on oled panel
}

void xs_SSD1306_begin(xsMachine *the)
{
	ssd1606 ssd = xsmcGetHostData(xsThis);
	CommodettoCoordinate xMin = xsmcToInteger(xsArg(0));
	CommodettoCoordinate yMin = xsmcToInteger(xsArg(1));
	CommodettoDimension width = xsmcToInteger(xsArg(2));
	CommodettoDimension height = xsmcToInteger(xsArg(3));

	if ((0 != xMin) || (0 != yMin) || (ssd->width != width) || (ssd->height != height))
		xsUnknownError("partial updates unsupported");

	ssd1306Begin(ssd, xMin, yMin, width, height);
}

void xs_SSD1306_send(xsMachine *the)
{
	ssd1606 ssd = xsmcGetHostData(xsThis);
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

	ssd1306Send((PocoPixel *)data, count, ssd);
}

void xs_SSD1306_end(xsMachine *the)
{
	ssd1306End(xsmcGetHostData(xsThis));
}

void ssd1306Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	ssd1606 ssd = refcon;

	doCmd(ssd, SSD1306_COLUMNADDR);
	doCmd(ssd, 0);   // Column start address (0 = reset)
	doCmd(ssd, ssd->width - 1); // Column end address (127 = reset)

	doCmd(ssd, SSD1306_PAGEADDR);
	doCmd(ssd, 0); // Page start address (0 = reset)

	doCmd(ssd, (ssd->height >> 3) - 1);	// Page end address

#if MODDEF_SSD1306_DITHER
	c_memset(ssd->ditherA, 0, sizeof(ssd->ditherA));
	c_memset(ssd->ditherB, 0, sizeof(ssd->ditherB));
	ssd->ditherPhase = 0;
#endif

#if MODDEF_SSD1306_SPI
	SCREEN_CS_DEACTIVE;		// inactive in sample when toggling DC
	SCREEN_DC_DATA;
	SCREEN_CS_ACTIVE;
#endif

	ssd->pixel = 0;
}

void ssd1306Continue(void *refcon)
{
	ssd1306End(refcon);
}

void ssd1306End(void *refcon)
{
}

void ssd1306Send(PocoPixel *pixels, int byteLength, void *refcon)
{
	ssd1606 ssd = refcon;
	uint8_t pixel, lines;

#if MODDEF_SSD1306_SPI
	modSPIFlush();
#endif

	if (byteLength < 0) byteLength = -byteLength;
	lines = (uint8_t)(byteLength / ssd->width);
	pixel = ssd->pixel;
	while (lines--) {
		uint8_t *out;
		uint8_t w;
#if MODDEF_SSD1306_DITHER
		int16_t *thisLineErrors, *nextLineErrors;

		if (ssd->ditherPhase) {
			c_memset(ssd->ditherB, 0, sizeof(ssd->ditherB));		// zero out next line
			thisLineErrors = ssd->ditherA + 2;
			nextLineErrors = ssd->ditherB + 2;
			ssd->ditherPhase = 0;
		}
		else {
			c_memset(ssd->ditherA, 0, sizeof(ssd->ditherA));		// zero out next line
			thisLineErrors = ssd->ditherB + 2;
			nextLineErrors = ssd->ditherA + 2;
			ssd->ditherPhase = 1;
		}
#endif

		if (0 == pixel) {		// start new group of 8 rows
			pixel = 1;
			c_memset(&ssd->buffer[kBufferSlop], 0, sizeof(ssd->buffer) - kBufferSlop);
		}

#if MODDEF_SSD1306_DITHER
		for (w = ssd->width, out = &ssd->buffer[kBufferSlop]; 0 != w; w--, out++) {
			int16_t thisPixel = *pixels++ + (thisLineErrors[0] >> 4);

			if (thisPixel >= 128) {
				*out |= pixel;
				thisPixel -= 255;
			}

			// Burkes
			thisLineErrors[ 1] += thisPixel << 2;
			thisLineErrors[ 2] += thisPixel << 1;
			nextLineErrors[-2] += thisPixel;
			nextLineErrors[-1] += thisPixel << 1;
			nextLineErrors[ 0] += thisPixel << 2;
			nextLineErrors[ 1] += thisPixel << 1;
			nextLineErrors[ 2] += thisPixel;

			thisLineErrors++;
			nextLineErrors++;
		}
#else
		for (w = ssd->width, out = &ssd->buffer[kBufferSlop]; 0 != w; w--, out++) {
			if (*pixels++ >= 128)
				*out |= pixel;
		}
#endif
		pixel <<= 1;

		if (0 == pixel)	 {	// flush this set of 8 lines
#if MODDEF_SSD1306_SPI
			modSPITx(&ssd->config.spi, &ssd->buffer[kBufferSlop], ssd->width);
#elif MODDEF_SSD1306_I2C
			modI2CWrite(&ssd->config.i2c, &ssd->buffer[kBufferSlop - 1], ssd->width + 1, false);		// true also works for stop boolean
#endif
		}
	}

	ssd->pixel = pixel;
}

void ssd1306AdaptInvalid(void *refcon, CommodettoRectangle r)
{
	ssd1606 ssd = refcon;

	r->x = 0;
	r->y = 0;
	r->w = ssd->width;
	r->h = ssd->height;
}

void xs_SSD1306_width(xsMachine *the)
{
	ssd1606 ssd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, ssd->width);
}

void xs_SSD1306_height(xsMachine *the)
{
	ssd1606 ssd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, ssd->height);
}

void doCmd(ssd1606 ssd, uint8_t cmd)
{
#if MODDEF_SSD1306_SPI
	SCREEN_CS_DEACTIVE;		// inactive in sample when toggling DC
	SCREEN_DC_COMMAND;
	SCREEN_CS_ACTIVE;
	modSPITx(&ssd->config.spi, &cmd, 1);
	modSPIFlush();
#elif MODDEF_SSD1306_I2C
	uint8_t data[2] = {0, cmd};
	uint8_t err = modI2CWrite(&ssd->config.i2c, data, 2, true);
	if (err) {
		xsMachine *the = ssd->the;
		xsUnknownError("command failed");
	}
#endif
}

#if MODDEF_SSD1306_SPI
void ssd1306ChipSelect(uint8_t active, modSPIConfiguration config)
{
	ssd1606 ssd = (ssd1606)(((char *)config) - offsetof(ssd1606Record, config.spi));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}
#endif

void xs_ssd1306_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}
