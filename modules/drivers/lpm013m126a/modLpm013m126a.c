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

#define __XS6ESP_GPIO__ 1
#include "lpm013m126a.h"
#include "xsmc.h"
#include "xsesp.h"
#include "modGPIO.h"
#include "stdlib.h"
#include "modSPI.h"
#include "commodettoBitmap.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"
#include "mc.defines.h"

#ifndef MODDEF_LPM013M126A_CS_PORT
	#define MODDEF_LPM013M126A_CS_PORT NULL
#endif
#ifndef MODDEF_LPM013M126A_HZ
	#define MODDEF_LPM013M126A_HZ (2000000)
#endif
#ifndef MODDEF_LPM013M126A_WIDTH
	#define MODDEF_LPM013M126A_WIDTH (176)
#endif
#ifndef MODDEF_LPM013M126A_HEIGHT
	#define MODDEF_LPM013M126A_HEIGHT (176)
#endif

#define DITHER 0

/*
	LPM013M126A			ESP8266
	DISPLAY					MCU
	----------			----------

	SCLK						GPIO 14

	SI							GPIO 13

	SCS 						GPIO 4

	DISP						3V3

	GND							GND

	VIN 						3V3
*/

#define LPMSIZE 176

#define redMask 0xE0		// 0b11100000
#define greenMask 0x1C		// 0b00011100
#define blueMask 0x3		// 0b00000011

#define redThresh 4
#define greenThresh 4
#define blueThresh 2

#define SCREEN_CS_DEACTIVE	modGPIOWrite(&lpm->cs, 0)
#define SCREEN_CS_ACTIVE	modGPIOWrite(&lpm->cs, 1)
#define SCREEN_CS_INIT		modGPIOInit(&lpm->cs, MODDEF_LPM013M126A_CS_PORT, MODDEF_LPM013M126A_CS_PIN, kModGPIOOutput); \
	SCREEN_CS_DEACTIVE

//Host data record.
struct lpm013m126aRecord {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;
	modGPIOConfigurationRecord	cs;

	uint16_t 	onRow;							//store what scanline row we are on
	uint16_t	bufferSize;

	uint8_t		*pixelBuffer;

	uint8_t		updateCycle;				//for toggling COM bit to avoid DC Bias

#if DITHER
	uint8_t		ditherPhase;
	int8_t		ditherA[LPMSIZE + 4];
	int8_t		ditherB[LPMSIZE + 4];
#endif
};
typedef struct lpm013m126aRecord lpm013m126aRecord;
typedef lpm013m126aRecord *lpm013m126a;

static void lpm013m126aChipSelect(uint8_t active, modSPIConfiguration config);
static void lpm_clear(lpm013m126a lpm);

static void lpm013m126aBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void lpm013m126aEnd(void *refcon);
static void lpm013m126aSend(PocoPixel *pixels, int byteLength, void *refcon);
static void lpm013m126aAdaptInvalid(void *refcon, CommodettoRectangle r);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	lpm013m126aBegin,
	lpm013m126aEnd,
	lpm013m126aEnd,
	lpm013m126aSend,
	lpm013m126aAdaptInvalid
};

void xs_LPM013M126A(xsMachine *the){
	lpm013m126a lpm;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		if (kCommodettoBitmapFormat != xsmcToInteger(xsVar(0)))
			xsUnknownError("bad format");
	}

	if (kCommodettoBitmapFormat != kCommodettoBitmapRGB332)
		xsUnknownError("rgb332 pixels required");

	if (MODDEF_LPM013M126A_WIDTH != LPMSIZE || MODDEF_LPM013M126A_HEIGHT != LPMSIZE)
		xsUnknownError("invalid dimensions");

	lpm = calloc(1, sizeof(lpm013m126aRecord));
	if (!lpm)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, lpm);
	lpm->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;

	lpm->onRow = 0;
	lpm->bufferSize = 0;

	SCREEN_CS_INIT;
	modSPIConfig(lpm->spiConfig, MODDEF_LPM013M126A_HZ, MODDEF_LPM013M126A_SPI_PORT,
			MODDEF_LPM013M126A_CS_PORT, MODDEF_LPM013M126A_CS_PIN, lpm013m126aChipSelect);
	modSPIInit(&lpm->spiConfig);

	lpm_clear(lpm);
}

void xs_lpm013m126a_get_c_dispatch(xsMachine *the) {
	xsResult = xsThis;
}

void xs_lpm013m126a_begin(xsMachine *the){
	lpm013m126a lpm = xsmcGetHostData(xsThis);
	uint16_t xMin = xsmcToInteger(xsArg(0));
	uint16_t yMin = xsmcToInteger(xsArg(1));
	uint16_t xMax = xMin + xsmcToInteger(xsArg(2));
	uint16_t yMax = yMin + xsmcToInteger(xsArg(3));

	if ((0 != xMin) || (MODDEF_LPM013M126A_WIDTH != xMax))
		xsUnknownError("partial scan line updates are not supported");

	lpm013m126aBegin(lpm, xMin, yMin, xMax - xMin, yMax - yMin);
}

void lpm013m126aBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	lpm013m126a lpm = refcon;

	lpm->onRow = y;

#if DITHER
	memset(lpm->ditherA, 0, sizeof(lpm->ditherA));
	memset(lpm->ditherB, 0, sizeof(lpm->ditherB));
	lpm->ditherPhase = 0;
#endif
}

void xs_lpm013m126a_send(xsMachine *the){
	lpm013m126a lpm = xsmcGetHostData(xsThis);
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

	lpm013m126aSend((PocoPixel *)data, count, lpm);
}

#define PIXEL_BYTES_PER_LINE (((MODDEF_LPM013M126A_WIDTH * 3) + 7) >> 3)
void lpm013m126aSend(PocoPixel *data, int count, void *refcon)
{
	lpm013m126a lpm = refcon;
	uint8_t *toSend;
	int lines;
	int size;
	uint8_t *out;

	modSPIActivateConfiguration(NULL);

	lines = (int)(count / MODDEF_LPM013M126A_WIDTH);
	size = 2 + ((PIXEL_BYTES_PER_LINE + 2) * lines);

	if (size > lpm->bufferSize){
		if (lpm->pixelBuffer) free(lpm->pixelBuffer);
		lpm->pixelBuffer = calloc(size + 10, 1);
		lpm->bufferSize = size;
	}

	toSend = lpm->pixelBuffer;
	lpm->updateCycle = !(lpm->updateCycle);
	toSend[0] = (lpm->updateCycle ? DATA_UPDATE_3BIT_COMH : DATA_UPDATE_3BIT_COML);
	toSend[1] = lpm->onRow + 1;
	out = toSend + 2;

	//Prader Pixel Packing
	uint32_t fourPixels;
	uint8_t outByte;
/* 
 * Dithering: Filter Lite	
 * 		  		 X    (1/2)
 * 		(1/4)  (1/4)
*/
#if DITHER
	int8_t *thisLineErrors, *nextLineErrors;
	uint8_t thisPixel;	
	uint8_t toAdd;
	for (int lineIndex = 0; lineIndex < 8; lineIndex++) { 		// 8 lines at a time
		if (lpm->ditherPhase) {
			memset(lpm->ditherB, 0, sizeof(lpm->ditherB));
			thisLineErrors = lpm->ditherA + 2;
			nextLineErrors = lpm->ditherB + 2;
			lpm->ditherPhase = 0;
		}
		else {
			memset(lpm->ditherA, 0, sizeof(lpm->ditherA));
			thisLineErrors = lpm->ditherB + 2;
			nextLineErrors = lpm->ditherA + 2;
			lpm->ditherPhase = 1;
		}
		for (int i = 0; i < MODDEF_LPM013M126A_WIDTH; i += 8) {  // 8 pixels per loop iteration
			fourPixels = *(uint32_t *)data;
			outByte = 0;
			toAdd = 128;
			// RGBRGBRG, BRGB...
			for (int j=0; j<4; j++) {
				// R
				thisPixel = ((fourPixels & redMask) + (thisLineErrors[0] & redMask)) >> 5;
				if (thisPixel >= redThresh) {
					outByte |= toAdd;
					thisPixel -= 7;
				}
				thisLineErrors[ 1] += (thisPixel >> 1) << 5;
				nextLineErrors[-1] += (thisPixel >> 2) << 5;
				nextLineErrors[ 0] += (thisPixel >> 2) << 5;
				toAdd = toAdd >> 1;
				// G
				thisPixel = ((fourPixels & greenMask) + (thisLineErrors[0] & greenMask)) >> 2;
				if (thisPixel >=  greenThresh) {
					outByte |= toAdd;
					thisPixel -= 7;
				}
				thisLineErrors[ 1] += (thisPixel >> 1) << 2;
				nextLineErrors[-1] += (thisPixel >> 2) << 2;
				nextLineErrors[ 0] += (thisPixel >> 2) << 2;
				toAdd = toAdd >> 1;
				if (toAdd == 0) { 		// end of RGBRGBRG
					*out++ = outByte;
					outByte = 0;
					toAdd = 128;
				}
				// B
				thisPixel = (fourPixels & blueMask) + (thisLineErrors[0] & blueMask);
				if (thisPixel >= blueThresh) {
					outByte |= toAdd;
					thisPixel -= 3;
				}
				thisLineErrors[ 1] += (thisPixel >> 1);		// no need to edit nextLineErrors since blue only starts with 2 bits
				toAdd = toAdd >> 1;	
				fourPixels = fourPixels >> 8;	
				thisLineErrors++;
				nextLineErrors++;
			}
			data += 4;
			fourPixels = *(uint32_t *)data;
			// ...RGBR, GBRGBRGB
			for (int j=0; j<4; j++) {
				// R
				thisPixel = ((fourPixels & redMask) + (thisLineErrors[0] & redMask)) >> 5;
				if (thisPixel >= redThresh) {
					outByte |= toAdd;
					thisPixel -= 7;
				}
				thisLineErrors[ 1] += (thisPixel >> 1) << 5;
				nextLineErrors[-1] += (thisPixel >> 2) << 5;
				nextLineErrors[ 0] += (thisPixel >> 2) << 5;
				toAdd = toAdd >> 1;
				if (toAdd == 0) { 		// end of BRGBRGBR
					*out++ = outByte;
					outByte = 0;
					toAdd = 128;
				}
				// G
				thisPixel = ((fourPixels & greenMask) + (thisLineErrors[0] & greenMask)) >> 2;
				if (thisPixel >=  greenThresh) {
					outByte |= toAdd;
					thisPixel -= 7;
				}
				thisLineErrors[ 1] += (thisPixel >> 1) << 2;
				nextLineErrors[-1] += (thisPixel >> 2) << 2;
				nextLineErrors[ 0] += (thisPixel >> 2) << 2;
				toAdd = toAdd >> 1;
				// B
				thisPixel = (fourPixels & blueMask) + (thisLineErrors[0] & blueMask);
				if (thisPixel >= blueThresh) {
					outByte |= toAdd;
					thisPixel -= 3;
				}
				thisLineErrors[ 1] += (thisPixel >> 1);
				toAdd = toAdd >> 1;	
				fourPixels = fourPixels >> 8;	
				thisLineErrors++;
				nextLineErrors++;
			}	
			data += 4;
			*out++ = outByte;
		}
		*out++ = 0;
		lpm->onRow++;
		*out++ = (uint8_t)lpm->onRow + 1;
	}	
#else 
	for (int lineIndex = 0; lineIndex < lines; lineIndex++) {
		for (int i = 0; i < MODDEF_LPM013M126A_WIDTH; i += 8) {  // 8 pixels per loop iteration
			fourPixels = *(uint32_t *)data;
			// RGBRGBRG
			outByte = (fourPixels & 128);
			outByte |= (fourPixels & 16) << 2;
			outByte |= (fourPixels & 2) << 4; 
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 3;
			outByte |= (fourPixels & 16) >> 1;
			outByte |= (fourPixels & 2) << 1;
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 6;
			outByte |= (fourPixels & 16) >> 4;							
			*out++ = outByte;
			// BRGBRGBR
			outByte = (fourPixels & 2) << 6;
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 1;
			outByte |= (fourPixels & 16) << 1;
			outByte |= (fourPixels & 2) << 3;
			data += 4;
			fourPixels = *(uint32_t *)data;
			outByte |= (fourPixels & 128) >> 4;
			outByte |= (fourPixels & 16) >> 2;
			outByte |= (fourPixels & 2);
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 7;
			*out++ = outByte;
			// GBRGBRGB
			outByte = (fourPixels & 16) << 3;
			outByte |= (fourPixels & 2) << 5;
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 2;
			outByte |= (fourPixels & 16);
			outByte |= (fourPixels & 2) << 2;
			fourPixels = fourPixels >> 8;
			outByte |= (fourPixels & 128) >> 5;
			outByte |= (fourPixels & 16) >> 3;
			outByte |= (fourPixels & 2) >> 1;
			*out++ = outByte;
			data += 4;
		}
		*out++ = 0;
		lpm->onRow++;
		*out++ = (uint8_t)lpm->onRow + 1;
	}
#endif
	modSPITx(&lpm->spiConfig, toSend, size);
}

void xs_lpm013m126a_end(xsMachine *the){
	lpm013m126a lpm = xsmcGetHostData(xsThis);
	lpm013m126aEnd(lpm);
}

void lpm013m126aEnd(void *refcon)
{
	modSPIActivateConfiguration(NULL);
}

void xs_LPM013M126A_destructor(void *data){
  if (data){
		if ( ((lpm013m126a)data)->pixelBuffer )
			free(((lpm013m126a)data)->pixelBuffer);
		free(data);
	}
}

void lpm013m126aHold(lpm013m126a lpm){
	lpm->updateCycle = !(lpm->updateCycle);
	uint8_t mode[2] = {0,0};
	if (lpm->updateCycle){
		mode[0] = NO_UPDATE_COMH;
	}else{
		mode[0] = NO_UPDATE_COML;
	}
	modSPITx(&lpm->spiConfig, mode, 2);
	modSPIFlush();
	modSPIActivateConfiguration(NULL);
}

static void lpm_clear(lpm013m126a lpm){
	uint8_t mode[2] = {0, 0};
	lpm->updateCycle = !(lpm->updateCycle);
	if (lpm->updateCycle){
		mode[0] = ALL_CLEAR_COMH;
	}else{
		mode[0] = ALL_CLEAR_COML;
	}

	modSPITx(&lpm->spiConfig, mode, 2);
	modSPIFlush();
	modSPIActivateConfiguration(NULL);
}

void xs_lpm013m126a_adaptInvalid(xsMachine *the)
{
	lpm013m126a lpm = xsmcGetHostData(xsThis);
	CommodettoRectangle invalid = xsmcGetHostChunk(xsArg(0));
	lpm013m126aAdaptInvalid(lpm, invalid);
}

void lpm013m126aAdaptInvalid(void *refcon, CommodettoRectangle r)
{
	lpm013m126a lpm = refcon;

	r->x = 0;
	r->w = MODDEF_LPM013M126A_WIDTH;
}

void lpm013m126aChipSelect(uint8_t active, modSPIConfiguration config)
{
	lpm013m126a lpm = (lpm013m126a)(((char *)config) - offsetof(lpm013m126aRecord, spiConfig));

	if (active)
		SCREEN_CS_ACTIVE;
	else
		SCREEN_CS_DEACTIVE;
}

void xs_lpm013m126a_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_lpm013m126a_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_LPM013M126A_WIDTH);
}

void xs_lpm013m126a_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_LPM013M126A_HEIGHT);
}
