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

#include "xsmc.h"
#include "xsHost.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "modSPI.h"

#ifndef MODDEF_DOTSTAR_BRIGHTNESS
	#define MODDEF_DOTSTAR_BRIGHTNESS (64)
#endif
#ifndef MODDEF_DOTSTAR_HZ
	#define MODDEF_DOTSTAR_HZ (20000000)
#endif

#ifdef __ZEPHYR__

/* TODO: K64F Test */
#define SPI_DEVICE				"SPI_0"

#elif gecko
	#define AUTO_CHIP_SELECT 0
	#define MOSI_PIN	0
	#define CLK_PIN		2
#else // linux (raspberry pi)
	// PI wiring
#endif

typedef struct {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;

	uint16_t					width;
	uint16_t					height;

	uint8_t						format;

	uint8_t						brightness[256];
} spiDisplayRecord, *spiDisplay;

static void dotstarChipSelect(uint8_t active, modSPIConfiguration config);

static void dotstarCommand(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count);

//@@ MODDEF
void setBrightness(spiDisplay sd, int brightness);		// 0 to 255

static void dotstarBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void dotstarContinue(void *refcon);
static void dotstarEnd(void *refcon);
static void dotstarSend(void *refcon, PocoPixel *pixels, uint32_t byteLength);
static void dotstarAdaptInvalid(void *refcon, CommodettoRectangle r);

static void dotstarSend_16LE(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch_16LE ICACHE_RODATA_ATTR = {
	dotstarBegin,
	dotstarContinue,
	dotstarEnd,
	dotstarSend_16LE,
	dotstarAdaptInvalid
};

void xs_DotStar_destructor(void *data)
{
	if (data) {
		spiDisplay sd = data;
		modSPIUninit(&sd->spiConfig);
		c_free(data);
	}
}

void xs_DotStar(xsMachine *the)
{
	int format;
	spiDisplay sd;
	int csPin;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		format = xsmcToInteger(xsVar(0));
	}
	else
		format = kCommodettoBitmapFormat;

	if (kCommodettoBitmapRGB565LE != format)
		xsUnknownError("bad format");

	sd = c_calloc(1, sizeof(spiDisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

	modSPIConfig(sd->spiConfig, MODDEF_DOTSTAR_HZ, MODDEF_DOTSTAR_SPI_PORT,
				 NULL, -1, dotstarChipSelect);
	modSPIInit(&sd->spiConfig);

	xsmcGet(xsVar(0), xsArg(0), xsID_width);
	sd->width = (uint16_t)xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_height);
	sd->height = (uint16_t)xsmcToInteger(xsVar(0));

	sd->format = (uint8_t)format;

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch_16LE;

	setBrightness(sd, MODDEF_DOTSTAR_BRIGHTNESS);
}

void xs_DotStar_begin(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	dotstarBegin(sd, x, y, w, h);
}

void xs_DotStar_send(xsMachine *the)
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

void xs_DotStar_end(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	dotstarEnd(sd);
}

void xs_DotStar_adaptInvalid(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoRectangle invalid;

	if (NULL == sd->dispatch->doAdaptInvalid)
		return;

	invalid = xsmcGetHostChunk(xsArg(0));
	dotstarAdaptInvalid(sd, invalid);
}

void xs_DotStar_pixelsToBytes(xsMachine *the)
{
	int count = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, ((count * kCommodettoPixelSize) + 7) >> 3);
}

void xs_DotStar_get_pixelFormat(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, sd->format);
}

void xs_DotStar_get_width(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, sd->width);
}

void xs_DotStar_get_height(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, sd->height);
}

void xs_DotStar_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

// caller provides little-endian pixels
//@@ could double buffer "buffer" to get some aysnc sending in this loop
void dotstarSend_16LE(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t *brightness = sd->brightness;

	if (byteLength < 0) byteLength = -byteLength;
	while (byteLength > 0) {
		uint8_t buffer[16 * 4];
		uint8_t *bgr = buffer;
		uint16_t count = (byteLength >= 32) ? 32 : byteLength;
		uint16_t counter = count >> 1;

		while (counter--) {
			PocoPixel pixel = *pixels++;
			uint8_t t;

			*bgr++ = 0xff;

			t = pixel & 0x1F;
			*bgr++ = brightness[(t << 3) | (t >> 3)];

			t = (pixel >> 5) & 0x3F;
			*bgr++ = brightness[(t << 2) | (t >> 4)];

			t = pixel >> 11;
			*bgr++ = brightness[(t << 3) | (t >> 3)];
		}

		modSPITx(&sd->spiConfig, (void *)buffer, count << 1);
		modSPIFlush();

		byteLength -= count;
	}
}

void dotstarChipSelect(uint8_t active, modSPIConfiguration config)
{
}

void dotstarBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint32_t zeros = 0;

	modSPITx(&sd->spiConfig, (void *)&zeros, sizeof(zeros));
	modSPIFlush();
}

void dotstarContinue(void *refcon)
{
	dotstarEnd(refcon);
}

void dotstarEnd(void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t bytes[16];
	int count = (sd->width + 15) / 16;

	c_memset(bytes, 0xff, sizeof(bytes));

	while (count) {
		int use = count;
		if (count > sizeof(bytes))
			count = sizeof(bytes);

		modSPITx(&sd->spiConfig, (void *)bytes, use);
		modSPIFlush();

		count -= use;
	}
}

void dotstarAdaptInvalid(void *refcon, CommodettoRectangle invalid)
{
	spiDisplay sd = refcon;

	invalid->x = 0;
	invalid->y = 0;
	invalid->w = sd->width;
	invalid->h = sd->height;
}

void setBrightness(spiDisplay sd, int brightness)		// 0 to 255
{
	int i;

	for (i = 0; i < 256; i++)
		sd->brightness[i] = (i * brightness) / 255;
}

