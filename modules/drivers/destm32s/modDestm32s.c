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

// modeled on Crystalfontz sample code for CFAP122250A0-0213
// which seems to use the DESTM32-S controller from good-display.com
//  https://www.crystalfontz.com/product/cfap122250a00213
// product number decoder ring: cf == crystalfontz.... ap == ??... 122250 == 122x250... a0 == ??.... 0213 == 2.13"
// assumes 4-wire SPI mode selected

#include "xsmc.h"

#include "xsHost.h"
#include "modGPIO.h"
#if defined(gecko)
	#define tryGeckoGPIOIrq	1
#endif

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "stddef.h"		// for offsetof macro
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "modSPI.h"

#ifdef __ets__
#if ESP32
	#define CS_PIN		15
	#define DC_PIN		2
	#define RST_PIN		0
	#define MOSI_PIN	13
	#define CLK_PIN		14
	#define BUSY_PIN	??
#else	// esp8266
//	#define CS_PIN		4
//	#define DC_PIN		2
//	#define RST_PIN		0
//	#define MOSI_PIN	13
//	#define CLK_PIN		14
//	#define BUSY_PIN	5

	// for gray + red - more or less ignoring BUSY and wire RESET to HIGH (3.3)
#endif

/*
	ARDUINO WIRING (pin numbers are Arduino):
		RED - power
		BLACK - ground

		YELLOW - 2 - EPD_BUSSEL (GROUND for 4-wire SPI mode)
		GREEN -  3 - EPD_BUSY
		BROWN - 4 - EPD_RESET
		PURPLE - 5 - EPD_DC

		BLUE - 10 - EPD_CS
		WHITE - 11 - MOSI
		ORANGE - 13 - SCK
	
	DSETM32S header:
		(board oriented with pins facing UP on and on the LEFT side

		X X - NC | YELLOW
		X X - BROWN | GREEN
		X X - BLUE | PURPLE
		X X - WHITE | ORANGE
		X X - NC | NC

		X X - NC | NC
		X X - NC | NC
		X X - NC | RED
		X X - NC | BLACK
		X X - NC | NC
*/

#ifndef MODDEF_SPI_MOSI_PORT
	#define MODDEF_SPI_MOSI_PORT NULL
#endif
#ifndef MODDEF_SPI_CLK_PORT
	#define MODDEF_SPI_CLK_PORT	NULL
#endif
#ifndef MODDEF_DESTM32S_CS_PORT
	#define MODDEF_DESTM32S_CS_PORT NULL
#endif
#ifndef MODDEF_DESTM32S_DC_PORT
	#define MODDEF_DESTM32S_DC_PORT NULL
#endif
#ifndef MODDEF_DESTM32S_RST_PORT
	#define MODDEF_DESTM32S_RST_PORT NULL
#endif
#ifndef MODDEF_DESTM32S_BUSY_PORT
	#define MODDEF_DESTM32S_BUSY_PORT NULL
#endif
#ifndef MODDEF_DESTM32S_HZ
	#define MODDEF_DESTM32S_HZ (500000)				// nominally 500000
#endif
#ifndef MODDEF_DESTM32S_WIDTH
	#define MODDEF_DESTM32S_WIDTH 122
#endif
#ifndef MODDEF_DESTM32S_HEIGHT
	#define MODDEF_DESTM32S_HEIGHT 250
#endif
#ifndef MODDEF_DESTM32S_CLEAR
	#define MODDEF_DESTM32S_CLEAR true
#endif
#ifndef MODDEF_DESTM32S_MODDABLE_THREE
	#define MODDEF_DESTM32S_MODDABLE_THREE 0
#endif
#ifndef MODDEF_DESTM32S_DITHER
	#define MODDEF_DESTM32S_DITHER (1)
#endif
#ifndef MODDEF_DESTM32S_FULL
	#define MODDEF_DESTM32S_FULL (0)
#endif

#define SCREEN_CS_ACTIVE	modGPIOWrite(&sd->cs, 0)
#define SCREEN_CS_DEACTIVE	modGPIOWrite(&sd->cs, 1)
#define SCREEN_CS_INIT		modGPIOInit(&sd->cs, MODDEF_DESTM32S_CS_PORT, MODDEF_DESTM32S_CS_PIN, kModGPIOOutput); \
		SCREEN_CS_DEACTIVE

#define SCREEN_DC_DATA		modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND	modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT		modGPIOInit(&sd->dc, MODDEF_DESTM32S_DC_PORT, MODDEF_DESTM32S_DC_PIN, kModGPIOOutput); \
		SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, MODDEF_DESTM32S_RST_PORT, MODDEF_DESTM32S_RST_PIN, kModGPIOOutput); \
		SCREEN_RST_DEACTIVE;

#else	// !__ets__
#ifdef __ZEPHYR__

/* TODO: K64F Test */
#define DC_PORT					"GPIO_2"
#define DC_PIN					12
#define RST_PORT				"GPIO_2"
#define RST_PIN					4
#define SPI_DEVICE				"SPI_0"

/* CS are handled in Zephyr kernel */
#define SCREEN_CS_ACTIVE
#define SCREEN_CS_DEACTIVE
#define SCREEN_CS_INIT

#define SCREEN_DC_DATA			modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&sd->dc, DC_PORT, DC_PIN, kModGPIOOutput); \
								SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, RST_PORT, RST_PIN, kModGPIOOutput); \
								SCREEN_RST_DEACTIVE

#elif gecko

#ifndef MODDEF_DESTM32S_CLEAR
	#define MODDEF_DESTM32S_CLEAR true
#endif

#define SCREEN_CS_ACTIVE		modGPIOWrite(&sd->cs, 0)
#define SCREEN_CS_DEACTIVE		modGPIOWrite(&sd->cs, 1)
#define SCREEN_CS_INIT			modGPIOInit(&sd->cs, MODDEF_DESTM32S_CS_PORT, MODDEF_DESTM32S_CS_PIN, kModGPIOOutput); \
		SCREEN_CS_DEACTIVE

#define SCREEN_DC_DATA			modGPIOWrite(&sd->dc, 1)
#define SCREEN_DC_COMMAND		modGPIOWrite(&sd->dc, 0)
#define SCREEN_DC_INIT			modGPIOInit(&sd->dc, MODDEF_DESTM32S_DC_PORT, MODDEF_DESTM32S_DC_PIN, kModGPIOOutput); \
								SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		modGPIOWrite(&sd->rst, 0)
#define SCREEN_RST_DEACTIVE		modGPIOWrite(&sd->rst, 1)
#define SCREEN_RST_INIT			modGPIOInit(&sd->rst, MODDEF_DESTM32S_RST_PORT, MODDEF_DESTM32S_RST_PIN, kModGPIOOutput); \
								SCREEN_RST_DEACTIVE

	
#else // linux (raspberry pi)
	// PI wiring
	#define SCREEN_CS_ACTIVE	GPIO_CLEAR(8)
	#define SCREEN_CS_DEACTIVE	GPIO_SET(8)
	#define SCREEN_CS_INIT		GPIO_INIT_OUTPUT(8); SCREEN_CS_DEACTIVE

	#define SCREEN_DC_DATA		GPIO_SET(24)
	#define SCREEN_DC_COMMAND	GPIO_CLEAR(24)
	#define SCREEN_DC_INIT		GPIO_INIT_OUTPUT(24); SCREEN_DC_DATA

#define SCREEN_RST_ACTIVE		GPIO_CLEAR(22)
#define SCREEN_RST_DEACTIVE		GPIO_SET(22)
#define SCREEN_RST_INIT			GPIO_INIT_OUTPUT(22); SCREEN_RST_DEACTIVE

#endif
#endif

#define kePaperBlackWhite (1)
#define kePaperBlackWhiteGrayRed (2)
#define kePaperBlackWhiteRed (3)

#ifdef MODDEF_DESTM32S_COLORS
	#define EPAPER MODDEF_DESTM32S_COLORS
#elif (122 == MODDEF_DESTM32S_WIDTH) && (250 == MODDEF_DESTM32S_HEIGHT)
	#define EPAPER kePaperBlackWhite
#elif (104 == MODDEF_DESTM32S_WIDTH) && (212 == MODDEF_DESTM32S_HEIGHT)
	#define EPAPER kePaperBlackWhiteGrayRed
#elif (128 == MODDEF_DESTM32S_WIDTH) && (296 == MODDEF_DESTM32S_HEIGHT)
	#define EPAPER kePaperBlackWhiteRed
#elif (400 == MODDEF_DESTM32S_WIDTH) && (300 == MODDEF_DESTM32S_HEIGHT)
	#define EPAPER kePaperBlackWhiteRed
#else
	#error
#endif

typedef struct {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;

	CommodettoDimension			updateWidth;

	uint8_t						full;
	uint8_t						powered;
	uint8_t						doPowerOff;
	uint8_t						forceFull;
	uint8_t						nextBuffer;
	uint8_t						lastLUT;

	modGPIOConfigurationRecord	cs;
	modGPIOConfigurationRecord	dc;
#ifdef MODDEF_DESTM32S_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif
	modGPIOConfigurationRecord	busy;

	uint32_t					alignNext;		// force bufferA and bufferB to be long aligned (required by SPI)
	uint8_t						bufferA[256];
	uint8_t						bufferB[256];

#if MODDEF_DESTM32S_DITHER
	uint8_t						dither;
	uint8_t						ditherPhase;
	int16_t						ditherA[MODDEF_DESTM32S_WIDTH + 4];
	int16_t						ditherB[MODDEF_DESTM32S_WIDTH + 4];
#endif

	char						*redP;
	uint8_t						red[(MODDEF_DESTM32S_WIDTH / 8) * MODDEF_DESTM32S_HEIGHT];
} spiDisplayRecord, *spiDisplay;

static void destm32sChipSelect(uint8_t active, modSPIConfiguration config);

static void destm32sInit_bw(spiDisplay sd);
static void destm32sInit_2g1r(spiDisplay sd);
static void destm32sInit_bw1r(spiDisplay sd);

static uint8_t destm32sWait(spiDisplay sd);
static uint8_t destm32sWait_2g1r(spiDisplay sd);
static uint8_t destm32sWait_bw1r(spiDisplay sd);
static void destm32sCommand(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count);

static void destm32sBegin_bw(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void destm32sBegin_2g1r(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void destm32sBegin_bw1r(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void destm32sContinue(void *refcon);
static void destm32sEnd_bw(void *refcon);
static void destm32sEnd_2g1r(void *refcon);
static void destm32sEnd_bw1r(void *refcon);

static void destm32sSend_bw(PocoPixel *pixels, int byteLength, void *refcon);
static void destm32sSend_2g1r(PocoPixel *pixels, int byteLength, void *refcon);
static void destm32sSend_bw1r(PocoPixel *pixels, int byteLength, void *refcon);
static void destm32sAdaptInvalid(void *refcon, CommodettoRectangle r);

static const PixelsOutDispatchRecord gPixelsOutDispatch_cfap122250a00213 ICACHE_RODATA_ATTR = {
	destm32sBegin_bw,
	destm32sContinue,
	destm32sEnd_bw,
	destm32sSend_bw,
	destm32sAdaptInvalid
};

static const PixelsOutDispatchRecord gPixelsOutDispatch_cfap104212B00213 ICACHE_RODATA_ATTR = {
	destm32sBegin_2g1r,
	destm32sContinue,
	destm32sEnd_2g1r,
	destm32sSend_2g1r,
	destm32sAdaptInvalid
};

static const PixelsOutDispatchRecord gPixelsOutDispatch_cfap128296D00290 ICACHE_RODATA_ATTR = {
	destm32sBegin_bw1r,
	destm32sContinue,
	destm32sEnd_bw1r,
	destm32sSend_bw1r,
	destm32sAdaptInvalid
};

void xs_destm32s_destructor(void *data)
{
	if (data) {
		spiDisplay sd = data;

		modGPIOUninit(&sd->cs);
		modGPIOUninit(&sd->dc);
#ifdef MODDEF_DESTM32S_RST_PIN
		modGPIOUninit(&sd->rst);
#endif
		modGPIOUninit(&sd->busy);
		modSPIUninit(&sd->spiConfig);
		c_free(data);
	}
}

void xs_destm32s(xsMachine *the)
{
	spiDisplay sd;
	uint8_t clear = MODDEF_DESTM32S_CLEAR && !MODDEF_DESTM32S_FULL;

#if EPAPER == kePaperBlackWhite
	if (kCommodettoBitmapGray256 != kCommodettoBitmapFormat)
		xsUnknownError("bad format");
#elif (EPAPER == kePaperBlackWhiteGrayRed || EPAPER == kePaperBlackWhiteRed)
	if (kCommodettoBitmapRGB332 != kCommodettoBitmapFormat)
		xsUnknownError("bad format");
#else
	xsUnknownError("bad format");
#endif

	sd = c_calloc(1, sizeof(spiDisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

#if EPAPER == kePaperBlackWhite
	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch_cfap122250a00213;
#elif EPAPER == kePaperBlackWhiteGrayRed
	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch_cfap104212B00213;
#elif EPAPER == kePaperBlackWhiteRed
	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch_cfap128296D00290;
#else
	xsUnknownError("unknown display");
#endif

	if (xsmcHas(xsArg(0), xsID_powerOff)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_powerOff);
		sd->doPowerOff = xsmcTest(xsVar(0));
	}

	SCREEN_CS_INIT;
	SCREEN_DC_INIT;

	modGPIOInit(&sd->busy, MODDEF_DESTM32S_BUSY_PORT, MODDEF_DESTM32S_BUSY_PIN, kModGPIOInput);

#ifdef MODDEF_DESTM32S_RST_PIN
	modGPIOInit(&sd->rst, MODDEF_DESTM32S_RST_PORT, MODDEF_DESTM32S_RST_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->rst, 1);
	modDelayMilliseconds(100);

	modGPIOWrite(&sd->rst, 0);
	modDelayMilliseconds(100);
	modGPIOWrite(&sd->rst, 1);
	modDelayMilliseconds(1000);
#endif

	modSPIConfig(sd->spiConfig, MODDEF_DESTM32S_HZ, MODDEF_DESTM32S_SPI_PORT,
			MODDEF_DESTM32S_CS_PORT, MODDEF_DESTM32S_CS_PIN, destm32sChipSelect);
	modSPIInit(&sd->spiConfig);

#if EPAPER == kePaperBlackWhite
	destm32sInit_bw(sd);
#elif EPAPER == kePaperBlackWhiteRed
	destm32sInit_bw1r(sd);
#endif

	if (xsmcHas(xsArg(0), xsID_clear)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_clear);
		clear = xsmcTest(xsVar(0));
	}

	sd->forceFull = MODDEF_DESTM32S_FULL;
#if MODDEF_DESTM32S_DITHER 
	sd->dither = true;
#endif

	if (clear)
		xsCall0(xsThis, xsID_refresh);
}

void xs_destm32s_begin(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	(sd->dispatch->doBegin)(sd, x, y, w, h);
}

void xs_destm32s_send(xsMachine *the)
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

void xs_destm32s_end(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	(sd->dispatch->doEnd)(sd);
}

void xs_destm32s_adaptInvalid(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoRectangle invalid;

	if (NULL == sd->dispatch->doAdaptInvalid)
		return;

	invalid = xsmcGetHostChunk(xsArg(0));
	(sd->dispatch->doAdaptInvalid)(sd, invalid);
}

void xs_destm32s_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_destm32s_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_DESTM32S_WIDTH);
}

void xs_destm32s_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_DESTM32S_HEIGHT);
}

void xs_destm32s_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void xs_destm32s_refresh(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	PocoPixel line[MODDEF_DESTM32S_WIDTH];
	uint16_t count;
	uint8_t forceFull = sd->forceFull;

	c_memset(line, 0xFF, MODDEF_DESTM32S_WIDTH);		// all white

	sd->forceFull = 1;
	(sd->dispatch->doBegin)(sd, 0, 0, MODDEF_DESTM32S_WIDTH, MODDEF_DESTM32S_HEIGHT);
		for (count = MODDEF_DESTM32S_HEIGHT; 0 != count; count--)
			(sd->dispatch->doSend)(line, MODDEF_DESTM32S_WIDTH, sd);
	(sd->dispatch->doEnd)(sd);
	sd->forceFull = forceFull;
}

void xs_destm32s_configure(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_full)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_full);
		sd->forceFull = xsmcToBoolean(xsVar(0));
	}

#if MODDEF_DESTM32S_DITHER
	if (xsmcHas(xsArg(0), xsID_dither)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_dither);
		sd->dither = xsmcToBoolean(xsVar(0));
	}
#endif
}

// caller provides 8-bit gray pixels. convert to 1-bit.
// pixels  written into 1-bit buffer in reverse order
void destm32sSend_bw(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	CommodettoDimension updateWidth = sd->updateWidth;
	uint16_t updateBytes = (updateWidth + 7) >> 3;
	uint16_t spaceInOutput = 0;
	uint8_t *mono = NULL;

	if (byteLength < 0) byteLength = -byteLength;
	for ( ; 0 != byteLength; byteLength -= updateWidth) {
		PocoPixel *nextPixels = pixels + updateWidth;
		int16_t i = updateBytes - 1, remain = updateWidth;

		if (spaceInOutput < updateWidth) {
			if (mono)
				modSPITx(&sd->spiConfig, sd->nextBuffer ? sd->bufferA : sd->bufferB, sizeof(sd->bufferA) - spaceInOutput);

			mono = sd->nextBuffer ? sd->bufferB : sd->bufferA;
			sd->nextBuffer ^= 1;
			spaceInOutput = sizeof(sd->bufferA);
		}

#if MODDEF_DESTM32S_DITHER
		if (sd->dither) {
			int16_t *thisLineErrors, *nextLineErrors;

			if (sd->ditherPhase) {
				thisLineErrors = sd->ditherA + 2;
				nextLineErrors = sd->ditherB + 2;
				sd->ditherPhase = 0;
			}
			else {
				thisLineErrors = sd->ditherB + 2;
				nextLineErrors = sd->ditherA + 2;
				sd->ditherPhase = 1;
			}

			uint8_t mask = 0x80 >> ((remain & 7) - 1);
			uint8_t outPixels = 0;
			while (remain--) {
				int16_t thisPixel = *pixels++ + (thisLineErrors[0] >> 3);

				if (thisPixel >= 128) {
					outPixels |= mask;
					thisPixel -= 255;
				}

				thisLineErrors[ 0]  = thisPixel;		// next next!
				thisLineErrors[+1] += thisPixel;
				thisLineErrors[+2] += thisPixel;

				nextLineErrors[-1] += thisPixel;
				nextLineErrors[ 0] += thisPixel;
				nextLineErrors[+1] += thisPixel;

				thisLineErrors++;
				nextLineErrors++;

				mask <<= 1;
				if (!mask) {
					mono[i--] = outPixels;	
					mask = 0x01;
					outPixels = 0;
				}
			}
		}
		else
#endif
		{
			if (remain & 7) {
				uint8_t mask = 0x80 >> ((remain & 7) - 1);
				mono[i] = 0;
				while (remain & 7) {
					if (*pixels++ & 0x80)
						mono[i] |= mask;
					mask <<= 1;
					remain--;
				}
				i--;
			}

			for (; remain >= 8; i--, remain > 0, pixels += 8, remain -= 8)
				mono[i] = ((pixels[0] & 0x80) >> 7) | ((pixels[1] & 0x80) >> 6) | ((pixels[2] & 0x80) >> 5) | ((pixels[3] & 0x80) >> 4) |
						((pixels[4] & 0x80) >> 3) | ((pixels[5] & 0x80) >> 2) | ((pixels[6] & 0x80) >> 1) | (pixels[7] & 0x80);
		}

		spaceInOutput -= updateBytes;
		mono += updateBytes;
		pixels = nextPixels;
	}

	modSPITx(&sd->spiConfig, sd->nextBuffer ? sd->bufferA : sd->bufferB, sizeof(sd->bufferA) - spaceInOutput);
}

uint8_t destm32sWait(spiDisplay sd)
{
#if tryGeckoGPIOIrq
	// wait for BUSY interrupt
	while (0 != modGPIORead(&sd->busy))
		waitForGPIO(MODDEF_DESTM32S_BUSY_PORT, MODDEF_DESTM32S_BUSY_PIN);
#else
	while (0 != modGPIORead(&sd->busy))
		modWatchDogReset();
#endif
	return 0;
}

void destm32sCommand(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count)
{
	SCREEN_DC_COMMAND;
   	modSPITxRx(&sd->spiConfig, &command, sizeof(command));		// could use modSPITx, but modSPITxRx is synchronous and callers rely on that

    if (count) {
    	SCREEN_DC_DATA;
        modSPITx(&sd->spiConfig, (uint8_t *)data, count);
    }

	modSPIActivateConfiguration(NULL);
}

#if MODDEF_DESTM32S_MODDABLE_THREE

void destm32sInit_bw(spiDisplay sd)
{
	uint8_t data[4] __attribute__((aligned(4)));

	destm32sWait(sd);
    destm32sCommand(sd, 0x12, NULL, 0);	// soft reset
	destm32sWait(sd);

	data[0] = 0x54;
    destm32sCommand(sd, 0x74, data, 1); //set analog block control
	data[0] = 0x3B;
    destm32sCommand(sd, 0x7E, data, 1); //set digital block control

	// Panel configuration, Gate selection for 2.13 inch
	data[0] = (250 - 1) & 0x00FF;
	data[1] = (250 - 1) >> 8;
	data[2] = 0x00;
    destm32sCommand(sd, 0x01, data, 3);

	// X decrease, Y decrease (sic)
	data[0] = 0x01; // Ram data entry mode
    destm32sCommand(sd, 0x11, data, 1);

	// X decrease, Y decrease
	data[0] = 0xd7;
	data[1] = 0xd6;
	data[2] = 0x9d;
	destm32sCommand(sd, 0x0c, data, 3);

	data[0] = 0x03;
    destm32sCommand(sd, 0x3C, data, 1); //BorderWavefrom

	// VCOM setting
	data[0] = 0xa8;
    destm32sCommand(sd, 0x2c, data, 1);

	data[0] = 0x15;
    destm32sCommand(sd, 0x03, data, 1);

	data[0] = 0x41;
	data[1] = 0xA8;
	data[2] = 0x32;
    destm32sCommand(sd, 0x04, data, 3);

	data[0] = 0x30;
    destm32sCommand(sd, 0x3A, data, 1);

	data[0] = 0x0A;
    destm32sCommand(sd, 0x3B, data, 1);

//	//dummy line per gate
//	data[0] = 0x1a;
//    destm32sCommand(sd, 0x3a, data, 1);
//
//	// Gate time setting
//	data[0] = 0x08;	 // 2us per line
//    destm32sCommand(sd, 0x3b, data, 1);
}

#else

void destm32sInit_bw(spiDisplay sd)
{
	uint8_t data[4] __attribute__((aligned(4)));

	// Panel configuration, Gate selection for 2.13 inch
	data[0] = (250 - 1) & 0x00FF;
	data[1] = (250 - 1) >> 8;
	data[2] = 0x00;
    destm32sCommand(sd, 0x01, data, 3);

	// X decrease, Y decrease
	data[0] = 0xd7;
	data[1] = 0xd6;
	data[2] = 0x9d;
    destm32sCommand(sd, 0x0c, data, 3);

	// VCOM setting
	data[0] = 0xa8;
    destm32sCommand(sd, 0x2c, data, 1);

	//dummy line per gate
	data[0] = 0x1a;
    destm32sCommand(sd, 0x3a, data, 1);

	// Gate time setting
	data[0] = 0x08;	 // 2us per line
    destm32sCommand(sd, 0x3b, data, 1);

	// X decrease, Y decrease (sic)
	data[0] = 0x01; // Ram data entry mode
    destm32sCommand(sd, 0x11, data, 1);
}
#endif

void destm32sChipSelect(uint8_t active, modSPIConfiguration config)
{
	spiDisplay sd = (spiDisplay)(((char *)config) - offsetof(spiDisplayRecord, spiConfig));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}

#if MODDEF_DESTM32S_MODDABLE_THREE
static const uint8_t LUT_Full_Update[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = {
0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7

0x03,0x03,0x00,0x00,0x02,                       // TP0 A~D RP0
0x09,0x09,0x00,0x00,0x02,                       // TP1 A~D RP1
0x03,0x03,0x00,0x00,0x02,                       // TP2 A~D RP2
0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6

};

static const uint8_t LUT_Partial_Update[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
0x80,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
0x40,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7

0x0A,0x00,0x00,0x00,0x00,                       // TP0 A~D RP0
0x00,0x00,0x00,0x00,0x00,                       // TP1 A~D RP1
0x00,0x00,0x00,0x00,0x00,                       // TP2 A~D RP2
0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6
};
#else
static const uint8_t LUT_Full_Update[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = {
	0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
	0x01, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t LUT_Partial_Update[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = {
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00
};
#endif
void destm32sBegin_bw(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint8_t data[2] __attribute__((aligned(4)));
	uint16_t byteWidth = (w + 7) >> 3;
	uint16_t yStart, yEnd;

	if (sd->forceFull && (w == MODDEF_DESTM32S_WIDTH) && (h == MODDEF_DESTM32S_HEIGHT) && (0 == x) && (0 == y)) {
		// full update
		if (1 != sd->lastLUT) {
			destm32sCommand(sd, 0x32, LUT_Full_Update, sizeof(LUT_Full_Update));
			sd->lastLUT = 1;
		}
		sd->full = 1;
	}
	else {
		// partial update
#if MODDEF_DESTM32S_MODDABLE_THREE
		if (2 != sd->lastLUT) {
			char d[1] __attribute__((aligned(4)));

			d[0] = 0x26;
			destm32sCommand(sd, 0x2c, d, 1);

			destm32sCommand(sd, 0x32, LUT_Partial_Update, sizeof(LUT_Partial_Update));
			sd->lastLUT = 2;

			char data[] __attribute__((aligned(4))) = {0, 0, 0, 0, 0x40, 0, 0} ;
			destm32sCommand(sd, 0x37, data, sizeof(data));
			data[0] = 0xc0;
			destm32sCommand(sd, 0x22, data, 1);
			destm32sCommand(sd, 0x20, NULL, 0);
			destm32sWait(sd);

			data[0] = 0x01;
			destm32sCommand(sd, 0x3C, data, 1); //BorderWavefrom
		}
#else
		if (2 != sd->lastLUT) {
			destm32sCommand(sd, 0x32, LUT_Partial_Update, sizeof(LUT_Partial_Update));
			sd->lastLUT = 2;
		}
#endif
		sd->full = 0;
	}

	if (!sd->powered) {
		// power on
		data[0] = 0xc0;
		destm32sCommand(sd, 0x22, data, 1);
		destm32sCommand(sd, 0x20, NULL, 0);
		sd->powered = 1;

		destm32sWait(sd);
	//@@ ESP was using delay MS here
	}

	//Set region X
	data[0] = x >> 3;
	data[1] = (x >> 3) + (byteWidth - 1);
	destm32sCommand(sd, 0x44, data, 2);

	//Set region Y
	yStart = y + (h - 1);
	yEnd = y;

	data[0] = (uint8_t)yStart;
	data[1] = yStart >> 8;
	data[2] = (uint8_t)yEnd;
	data[3] = yEnd >> 8;
	destm32sCommand(sd, 0x45, data, 4);

	//Set origin X
	data[0] = x >> 3;
	destm32sCommand(sd, 0x4e, data, 1);

	//Set origin Y
	data[0] = (uint8_t)yStart;
	data[1] = yStart >> 8;
	destm32sCommand(sd, 0x4f, data, 2);

	// write to RAM
	destm32sWait(sd);

	SCREEN_DC_COMMAND;

	data[0] = 0x24;
   	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	SCREEN_DC_DATA;

	sd->updateWidth = w;

#if MODDEF_DESTM32S_DITHER
	if (sd->dither) {
		c_memset(sd->ditherA, 0, sizeof(sd->ditherA));
		c_memset(sd->ditherB, 0, sizeof(sd->ditherB));
		sd->ditherPhase = 0;
	}
#endif
}

void destm32sContinue(void *refcon)
{
	//@@ multiple update areas allowed for a single update cycle?
}

#if MODDEF_DESTM32S_MODDABLE_THREE
void destm32sEnd_bw(void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	modSPIActivateConfiguration(NULL);

	data[0] = 0x0C;
	destm32sCommand(sd, 0x22, data, 1);

	destm32sCommand(sd, 0x20, NULL, 0);
	destm32sWait(sd);
	destm32sCommand(sd, 0xff, NULL, 0);

	if (sd->full || sd->doPowerOff) {
		destm32sWait(sd);	// wait until not busy
		// power off
		data[0] = 0x03;
		destm32sCommand(sd, 0x22, data, 1);
		destm32sCommand(sd, 0x20, NULL, 0);

		sd->powered = 0;

		destm32sWait(sd);	// wait for power off to complete
	}
}
#else
void destm32sEnd_bw(void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	modSPIActivateConfiguration(NULL);

	data[0] = sd->full ? 0xc7 : 0x04;
	destm32sCommand(sd, 0x22, data, 1);

	destm32sCommand(sd, 0x20, NULL, 0);
	destm32sCommand(sd, 0xff, NULL, 0);

	if (sd->full || sd->doPowerOff) {
		destm32sWait(sd);	// wait until not busy
		// power off
		data[0] = 0xc3;
		destm32sCommand(sd, 0x22, data, 1);
		destm32sCommand(sd, 0x20, NULL, 0);

		sd->powered = 0;

		destm32sWait(sd);	// wait for power off to complete
	}
}
#endif

void destm32sAdaptInvalid(void *refcon, CommodettoRectangle invalid)
{
	invalid->x = 0;
	invalid->y = 0;
	invalid->w = MODDEF_DESTM32S_WIDTH;
	invalid->h = MODDEF_DESTM32S_HEIGHT;
}

/*
	2-bit gray, 1-bit red
*/

// Note: These commands are also used by the 1-bit bw, 1-bit red driver
#define SET_PANEL   0x00
#define SET_PWR_REG 0x01
#define PWR_OFF     0x02
#define PWR_ON      0x04
#define BTST        0x06
#define REFRESH_CMD 0x12
#define SET_PLL     0x30
#define TEMP_SENSE  0x41
#define SET_VCOM    0x50
#define SET_RES     0x61
#define VCOM_DC     0x82
#define DATA_STOP   0x11
#define DATA_START1 0x10
#define DATA_START2 0x13

#define cmd_00_default 0xc3
#define cmd_30_default 0x2a
#define cmd_41_default 0x00
#define cmd_50_default 0x27
#define cmd_82_default 0x0a
#define vcom_fix 0x0a

// LUTs from 2.9 grey + red
static const uint8_t lut_vcom0[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14, 0x01 ,0x01 ,0x05 ,0x07 ,0x05 ,0x0C ,0x0C ,0x0A ,0x04 ,0x04 ,0x0A ,0x05 ,0x07 ,0x05 };
static const uint8_t lut_w[]     ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14, 0x01 ,0x01 ,0x45 ,0x07 ,0x05 ,0x8C ,0x4C ,0x0A ,0x84 ,0x44 ,0x0A ,0x85 ,0x07 ,0x05 };
static const uint8_t lut_b[]     ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14, 0x01 ,0x01 ,0x05 ,0x87 ,0x05 ,0x8C ,0x4C ,0x0A ,0x84 ,0x44 ,0x0A ,0x05 ,0x47 ,0x05 };
static const uint8_t lut_g1[]    ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x94, 0x81 ,0x01 ,0x05 ,0x87 ,0x05 ,0x8C ,0x4C ,0x0A ,0x84 ,0x44 ,0x0A ,0x05 ,0x07 ,0x05 };
static const uint8_t lut_g2[]    ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x94, 0x81 ,0x01 ,0x05 ,0x87 ,0x05 ,0x8C ,0x4C ,0x0A ,0x84 ,0x44 ,0x0A ,0x05 ,0x07 ,0x05 };
static const uint8_t lut_vcom1[] ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x01, 0x09 ,0x23 ,0x06 ,0x04 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };
static const uint8_t lut_red0[]  ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x81, 0x49 ,0x23 ,0x46 ,0x44 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };
static const uint8_t lut_red1[]  ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x01, 0x09 ,0x23 ,0x06 ,0x04 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };

uint8_t destm32sWait_2g1r(spiDisplay sd)
{
	uint32_t timeoutMS = modMilliseconds() + 25 * 1000;

	while (0 == modGPIORead(&sd->busy)) {
#if tryGeckoGPIOIrq
		// wait for BUSY interrupt or it will keep power on when sleeping
		while (0 != modGPIORead(&sd->busy))
			waitForGPIO(MODDEF_DESTM32S_BUSY_PORT, MODDEF_DESTM32S_BUSY_PIN);
#else
		modWatchDogReset();
#endif
		if (modMilliseconds() >= timeoutMS) {
			xmodLog("time out");
			return 1;
		}
		modDelayMilliseconds(10);
	}
	return 0;
}

void destm32sInit_2g1r(spiDisplay sd)
{
	uint8_t data[4] __attribute__((aligned(4)));

	destm32sWait_2g1r(sd);

	data[0] = 0x07;
	data[1] = 0x00;
	data[2] = 0x0a;
	data[3] = 0x00;
	destm32sCommand(sd, SET_PWR_REG, data, 4);

	data[0] = 0x07;
	data[1] = 0x06;
	data[2] = 0x05;
	destm32sCommand(sd, BTST, data, 3);

	destm32sCommand(sd, PWR_ON, NULL, 0);

	data[0] = 0xc3;
	destm32sCommand(sd, SET_PANEL, data, 1);

	data[0] = 0x27;
	destm32sCommand(sd, SET_VCOM, data, 1);

	data[0] = 0x2a;
	destm32sCommand(sd, SET_PLL, data, 1);

	data[0] = 0x00;
	destm32sCommand(sd, TEMP_SENSE, data, 1);

	data[0] = 0x68;
	data[1] = 0x00;
	data[2] = 0xd4;
	destm32sCommand(sd, SET_RES, data, 3);

	destm32sCommand(sd, 0x20, lut_vcom0, sizeof(lut_vcom0));
	destm32sCommand(sd, 0x21, lut_w, sizeof(lut_w));
	destm32sCommand(sd, 0x22, lut_b, sizeof(lut_b));
	destm32sCommand(sd, 0x23, lut_g1, sizeof(lut_g1));
	destm32sCommand(sd, 0x24, lut_g2, sizeof(lut_g2));
	destm32sCommand(sd, 0x25, lut_vcom1, sizeof(lut_vcom1));
	destm32sCommand(sd, 0x26, lut_red0, sizeof(lut_red0));
	destm32sCommand(sd, 0x27, lut_red1, sizeof(lut_red1));

	data[0] = vcom_fix;
	destm32sCommand(sd, VCOM_DC, data, 1);
}

void destm32sBegin_2g1r(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	destm32sInit_2g1r(sd);

	// Write the image data out to the display

	SCREEN_DC_COMMAND;

	data[0] = DATA_START1;
   	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	SCREEN_DC_DATA;

	sd->redP = sd->red;
}

void pixelToGrayAndRed(PocoPixel pixel, uint8_t *gray, uint8_t *red)
{
	uint8_t pixel_r = (pixel & 0xE0) >> 5;
	uint8_t pixel_g = (pixel & 0x1C) >> 2;
	uint8_t pixel_b = ((pixel & 0x03) << 1) | ((pixel & 0x02) >> 1);
	uint16_t delta_black = (pixel_r * pixel_r) + (pixel_g * pixel_g) + (pixel_b * pixel_b);
	uint16_t delta_white = ((7 - pixel_r) * (7 - pixel_r)) + ((7 - pixel_g) * (7 - pixel_g)) + ((7 - pixel_b) * (7 - pixel_b));
	uint16_t delta_gray = ((3 - pixel_r) * (3 - pixel_r)) + ((3 - pixel_g) * (3 - pixel_g)) + ((3 - pixel_b) * (3 - pixel_b));

	if ((delta_black <= delta_white) && (delta_black <= delta_gray))
		*gray = 0xC0;
	else if ((delta_white <= delta_black) && (delta_white <= delta_gray))
		*gray = 0x00;
	else
		*gray = 0x80;

	*red = ((pixel_r >= 5) && (pixel_r > pixel_g) && (pixel_r > pixel_b));
}

void destm32sSend_2g1r(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;

	if (byteLength < 0) byteLength = -byteLength;
	while (byteLength > 0) {
		uint8_t i;
		uint8_t bits[(104 * 2) / 8];		// 26 bytes

		for (i = 0; i < 104; i += 8) {
			uint8_t grayOutput, redOutput;
			uint8_t gray, red;

			pixelToGrayAndRed(pixels[0], &gray, &red);
			grayOutput = gray;
			redOutput = red << 7;

			pixelToGrayAndRed(pixels[1], &gray, &red);
			grayOutput |= gray >> 2;
			redOutput |= red << 6;

			pixelToGrayAndRed(pixels[2], &gray, &red);
			grayOutput |= gray >> 4;
			redOutput |= red << 5;

			pixelToGrayAndRed(pixels[3], &gray, &red);
			grayOutput |= gray >> 6;
			redOutput |= red << 4;

			bits[i >> 2] = grayOutput;

			pixelToGrayAndRed(pixels[4], &gray, &red);
			grayOutput = gray;
			redOutput |= red << 3;

			pixelToGrayAndRed(pixels[5], &gray, &red);
			grayOutput |= gray >> 2;
			redOutput |= red << 2;

			pixelToGrayAndRed(pixels[6], &gray, &red);
			grayOutput |= gray >> 4;
			redOutput |= red << 1;

			pixelToGrayAndRed(pixels[7], &gray, &red);
			grayOutput |= gray >> 6;
			redOutput |= red;

			bits[(i >> 2) + 1] = grayOutput;
			*sd->redP++ = redOutput;

			pixels += 8;
		}

		modSPITx(&sd->spiConfig, bits, sizeof(bits));
		modSPIFlush();

		byteLength -= MODDEF_DESTM32S_WIDTH;
	}
}

void destm32sEnd_2g1r(void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	destm32sCommand(sd, DATA_STOP, NULL, 0);

	// Write red data
	SCREEN_DC_COMMAND;

	data[0] = DATA_START2;
   	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	SCREEN_DC_DATA;

	modSPITx(&sd->spiConfig, sd->red, sizeof(sd->red));
	modSPIFlush();

	destm32sCommand(sd, DATA_STOP, NULL, 0);

	// Display new image
	destm32sCommand(sd, REFRESH_CMD, NULL, 0);
	destm32sWait_2g1r(sd);

	// power off
//	destm32sCommand(sd, PWR_OFF, NULL, 0);
}

/*
	1-bit bw, 1-bit red
*/

// LUTs for CFAP128296D0-0290
static const uint8_t VCOM_LUT_LUTC[]		ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14,0x01,0x01,0x05,0x07,0x05,0x0C,0x0C,0x0A,0x04,0x04,0x0A,0x05,0x07,0x05 };
static const uint8_t W2W_LUT_LUTWW[]		ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14,0x01,0x01,0x45,0x07,0x05,0x8C,0x4C,0x0A,0x84,0x44,0x0A,0x85,0x07,0x05 };
static const uint8_t B2W_LUT_LUTBW_LUTR[]	ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x14,0x01,0x01,0x05,0x87,0x05,0x8C,0x4C,0x0A,0x84,0x44,0x0A,0x05,0x47,0x05 };
static const uint8_t W2B_LUT_LUTWB_LUTW[]	ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x94,0x81,0x01,0x05,0x87,0x05,0x8C,0x4C,0x0A,0x84,0x44,0x0A,0x05,0x07,0x05 };
static const uint8_t B2B_LUT_LUTBB_LUTB[]	ICACHE_RODATA_ATTR __attribute__((aligned(4))) = { 0x94,0x81,0x01,0x05,0x87,0x05,0x8C,0x4C,0x0A,0x84,0x44,0x0A,0x05,0x07,0x05 };

uint8_t destm32sWait_bw1r(spiDisplay sd)
{
	return destm32sWait_2g1r(sd);
}

void destm32sCommand_bw1r(spiDisplay sd, uint8_t command, const uint8_t *data, uint16_t count)
{
	destm32sWait_bw1r(sd);
	destm32sCommand(sd, command, data, count);
}

void destm32sInit_bw1r(spiDisplay sd)
{
	uint8_t data[5] __attribute__((aligned(4)));

	data[0] = 0x03;
	data[1] = 0x00;
	data[2] = 0x0a;
	data[3] = 0x00;
	data[4] = 0x03;
	destm32sCommand_bw1r(sd, SET_PWR_REG, data, 5);

	data[0] = 0x17;
	data[1] = 0x17;
	data[2] = 0x17;
	destm32sCommand_bw1r(sd, BTST, data, 3);

	destm32sCommand_bw1r(sd, PWR_ON, NULL, 0);

	data[0] = 0x83;
	destm32sCommand_bw1r(sd, SET_PANEL, data, 1);

	data[0] = 0x87;
	destm32sCommand_bw1r(sd, SET_VCOM, data, 1);

	data[0] = 0x29;
	destm32sCommand_bw1r(sd, SET_PLL, data, 1);

	data[0] = MODDEF_DESTM32S_WIDTH>>8;
	data[1] = MODDEF_DESTM32S_WIDTH&0xf8;
	data[2] = MODDEF_DESTM32S_HEIGHT>>8;
	data[3] = MODDEF_DESTM32S_HEIGHT&0xff;
	destm32sCommand_bw1r(sd, SET_RES, data, 4);

	data[0] = 0x0a;
	destm32sCommand_bw1r(sd, VCOM_DC, data, 1);

	destm32sCommand_bw1r(sd, 0x20, VCOM_LUT_LUTC, sizeof(VCOM_LUT_LUTC));
	destm32sCommand_bw1r(sd, 0x21, W2W_LUT_LUTWW, sizeof(W2W_LUT_LUTWW));
	destm32sCommand_bw1r(sd, 0x22, B2W_LUT_LUTBW_LUTR, sizeof(B2W_LUT_LUTBW_LUTR));
	destm32sCommand_bw1r(sd, 0x23, W2B_LUT_LUTWB_LUTW, sizeof(W2B_LUT_LUTWB_LUTW));
	destm32sCommand_bw1r(sd, 0x24, B2B_LUT_LUTBB_LUTB, sizeof(B2B_LUT_LUTBB_LUTB));
}

void destm32sBegin_bw1r(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	destm32sWait_bw1r(sd);
	
	//SCREEN_CS_ACTIVE;
	
	SCREEN_DC_COMMAND;

	data[0] = DATA_START1;
	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	SCREEN_DC_DATA;

	sd->redP = sd->red;
}

void pixelToMonoAndRed(PocoPixel pixel, uint8_t *mono, uint8_t *red)
{
	uint8_t pixel_r = (pixel & 0xE0) >> 5;
	uint8_t pixel_g = (pixel & 0x1C) >> 2;
	uint8_t pixel_b = ((pixel & 0x03) << 1) | ((pixel & 0x02) >> 1);
	uint8_t gray = ((pixel_r << 1) + pixel_r + (pixel_g << 2) + pixel_b) >> 3;
	
	*mono = (gray > 4 ? 0 : 1);

	*red = ((pixel_r >= 5) && (pixel_r > pixel_g) && (pixel_r > pixel_b));
}

void destm32sSend_bw1r(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;

	if (byteLength < 0) byteLength = -byteLength;
	while (byteLength > 0) {
		uint16_t i;
		uint8_t bits[MODDEF_DESTM32S_WIDTH / 8];

		for (i = 0; i < MODDEF_DESTM32S_WIDTH; i += 8) {
			uint8_t monoOutput, redOutput;
			uint8_t mono, red;

			pixelToMonoAndRed(pixels[0], &mono, &red);
			monoOutput = mono << 7;
			redOutput = red << 7;

			pixelToMonoAndRed(pixels[1], &mono, &red);
			monoOutput |= mono << 6;
			redOutput |= red << 6;

			pixelToMonoAndRed(pixels[2], &mono, &red);
			monoOutput |= mono << 5;
			redOutput |= red << 5;

			pixelToMonoAndRed(pixels[3], &mono, &red);
			monoOutput |= mono << 4;
			redOutput |= red << 4;

			pixelToMonoAndRed(pixels[4], &mono, &red);
			monoOutput |= mono << 3;
			redOutput |= red << 3;
			
			pixelToMonoAndRed(pixels[5], &mono, &red);
			monoOutput |= mono << 2;
			redOutput |= red << 2;
			
			pixelToMonoAndRed(pixels[6], &mono, &red);
			monoOutput |= mono << 1;
			redOutput |= red << 1;
			
			pixelToMonoAndRed(pixels[7], &mono, &red);
			monoOutput |= mono;
			redOutput |= red;
			
			bits[i >> 3] = monoOutput;
			*sd->redP++ = redOutput;

			pixels += 8;
		}

		// Send the monochrome line
		modSPITx(&sd->spiConfig, bits, sizeof(bits));
		modSPIFlush();

		byteLength -= MODDEF_DESTM32S_WIDTH;
	}
}

void destm32sEnd_bw1r(void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t data[4] __attribute__((aligned(4)));

	// Write red data
	SCREEN_DC_COMMAND;

	data[0] = DATA_START2;
	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	SCREEN_DC_DATA;

	modSPITx(&sd->spiConfig, sd->red, sizeof(sd->red));
	modSPIFlush();

	SCREEN_DC_COMMAND;

	data[0] = DATA_STOP;
	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	data[0] = REFRESH_CMD;
	modSPITx(&sd->spiConfig, data, 1);
	modSPIFlush();

	//SCREEN_CS_DEACTIVE;
}
