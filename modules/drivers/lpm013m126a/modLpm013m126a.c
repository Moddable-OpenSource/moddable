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

#include "lpm013m126a.h"
#include "xsmc.h"
#include "xsHost.h"
#include "modGPIO.h"
#include "modSPI.h"
#include "commodettoBitmap.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"
#include "mc.defines.h"

#define LPMSIZE (176)

#ifndef MODDEF_LPM013M126A_CS_PORT
	#define MODDEF_LPM013M126A_CS_PORT NULL
#endif
#ifndef MODDEF_LPM013M126A_HZ
	#define MODDEF_LPM013M126A_HZ (2500000)
#endif
#ifndef MODDEF_LPM013M126A_WIDTH
	#define MODDEF_LPM013M126A_WIDTH LPMSIZE
#endif
#ifndef MODDEF_LPM013M126A_HEIGHT
	#define MODDEF_LPM013M126A_HEIGHT LPMSIZE
#endif
#ifndef MODDEF_LPM013M126A_DITHER
	#define MODDEF_LPM013M126A_DITHER (0)
#endif
#if kCommodettoBitmapFormat != kCommodettoBitmapRGB332
	#error rgb332 pixels required
#endif
#if (MODDEF_LPM013M126A_WIDTH != LPMSIZE) || (MODDEF_LPM013M126A_HEIGHT != LPMSIZE)
	#error invalid dimensions
#endif


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

#define redMask 0xE0		// 0b11100000
#define greenMask 0x1C		// 0b00011100
#define blueMask 0x3		// 0b00000011

#define redThresh 4
#define greenThresh 4
#define blueThresh 2

#ifdef MODDEF_LPM013M126A_CS_PIN
	#define SCREEN_CS_DEACTIVE	modGPIOWrite(&lpm->cs, 0)
	#define SCREEN_CS_ACTIVE	modGPIOWrite(&lpm->cs, 1)
	#define SCREEN_CS_INIT		modGPIOInit(&lpm->cs, MODDEF_LPM013M126A_CS_PORT, MODDEF_LPM013M126A_CS_PIN, kModGPIOOutput); \
		SCREEN_CS_DEACTIVE
#else
	#define SCREEN_CS_DEACTIVE
	#define SCREEN_CS_ACTIVE
	#define SCREEN_CS_INIT
#endif

#if !MODDEF_LPM013M126A_DITHER
// deliberately not setting ICACHE_XS6RO_ATTR to allow fast access on ESP8266
static uint8_t gRGB332to111[256] = {
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07
};
#endif

//Host data record.
struct lpm013m126aRecord {
	PixelsOutDispatch			dispatch;

	modSPIConfigurationRecord	spiConfig;
	modGPIOConfigurationRecord	cs;

	uint16_t 	onRow;							//store what scanline row we are on
	uint16_t	bufferSize;

	uint8_t		*pixelBuffer;

	uint8_t		updateCycle;				//for toggling COM bit to avoid DC Bias

#if MODDEF_LPM013M126A_DITHER
	uint8_t		ditherPhase;
	int8_t		ditherA[MODDEF_LPM013M126A_WIDTH + 4];
	int8_t		ditherB[MODDEF_LPM013M126A_WIDTH + 4];
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

	lpm = c_calloc(1, sizeof(lpm013m126aRecord));
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
		xsUnknownError("partial updates unsupported");

	lpm013m126aBegin(lpm, xMin, yMin, xMax - xMin, yMax - yMin);
}

void lpm013m126aBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	lpm013m126a lpm = refcon;

	lpm->onRow = y + 1;

#if MODDEF_LPM013M126A_DITHER
	c_memset(lpm->ditherA, 0, sizeof(lpm->ditherA));
	c_memset(lpm->ditherB, 0, sizeof(lpm->ditherB));
	lpm->ditherPhase = 0;
#endif
}

void xs_lpm013m126a_send(xsMachine *the){
	lpm013m126a lpm = xsmcGetHostData(xsThis);
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

	lpm013m126aSend((PocoPixel *)data, count, lpm);
}

#define PIXEL_BYTES_PER_LINE (((MODDEF_LPM013M126A_WIDTH * 3) + 7) >> 3)
void lpm013m126aSend(PocoPixel *data, int count, void *refcon)
{
	lpm013m126a lpm = refcon;
	int lines;
	int size;
	uint8_t *out;

	if (count < 0) count = -count;
	modSPIActivateConfiguration(NULL);

	lines = count / MODDEF_LPM013M126A_WIDTH;
	size = 2 + ((PIXEL_BYTES_PER_LINE + 2) * lines);

	if (size > lpm->bufferSize){
		if (lpm->pixelBuffer)
			c_free(lpm->pixelBuffer);
		lpm->pixelBuffer = c_malloc(size);
		if (!lpm->pixelBuffer)
			return;
		lpm->bufferSize = size;
	}

	lpm->updateCycle = !(lpm->updateCycle);
	out = lpm->pixelBuffer;
	*out++ = (lpm->updateCycle ? DATA_UPDATE_3BIT_COMH : DATA_UPDATE_3BIT_COML);
	*out++ = lpm->onRow;

	//Prader Pixel Packing
	uint32_t fourPixels;
	uint8_t outByte;
/* 
 * Dithering: Filter Lite	
 * 		  		 X    (1/2)
 * 		(1/4)  (1/4)
*/
#if MODDEF_LPM013M126A_DITHER
	int8_t *thisLineErrors, *nextLineErrors;
	uint8_t thisPixel;	
	uint8_t toAdd;
//@@ assumes 8 lines....
	for (int lineIndex = 0; lineIndex < 8; lineIndex++) { 		// 8 lines at a time
		if (lpm->ditherPhase) {
			c_memset(lpm->ditherB, 0, sizeof(lpm->ditherB));
			thisLineErrors = lpm->ditherA + 2;
			nextLineErrors = lpm->ditherB + 2;
			lpm->ditherPhase = 0;
		}
		else {
			c_memset(lpm->ditherA, 0, sizeof(lpm->ditherA));
			thisLineErrors = lpm->ditherB + 2;
			nextLineErrors = lpm->ditherA + 2;
			lpm->ditherPhase = 1;
		}
		for (int i = 0; i < MODDEF_LPM013M126A_WIDTH; i += 8) {  // 8 pixels per loop iteration
			fourPixels = *(uint32_t *)data;
			data += 4;
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
			fourPixels = *(uint32_t *)data;
			data += 4;
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
			*out++ = outByte;
		}
		*out++ = 0;
		lpm->onRow++;
		*out++ = (uint8_t)lpm->onRow;
	}	
#else 
	while (lines--) {
		for (int i = MODDEF_LPM013M126A_WIDTH / 8; i > 0; i -= 1, out += 3, data += 8) {  // 8 pixels per loop iteration
			uint8_t t;

			// RGBRGBRG
			t = *(gRGB332to111 + data[0]);
			outByte = t << 5;

			t = *(gRGB332to111 + data[1]);
			outByte |= t << 2;

			t = *(gRGB332to111 + data[2]);
			outByte |= t >> 1;

			out[0] = outByte;

			// BRGBRGBR
			outByte = t << 7;

			t = *(gRGB332to111 + data[3]);
			outByte |= t << 4;

			t = *(gRGB332to111 + data[4]);
			outByte |= t << 1;

			t = *(gRGB332to111 + data[5]);
			outByte |= t >> 2;

			out[1] = outByte;

			// GBRGBRGB
			outByte = t << 6;

			t = *(gRGB332to111 + data[6]);
			outByte |= t << 3;

			t = *(gRGB332to111 + data[7]);
			outByte |= t;

			out[2] = outByte;
		}
		*out++ = 0;
		lpm->onRow++;
		*out++ = (uint8_t)lpm->onRow;
	}
#endif
	modSPITx(&lpm->spiConfig, lpm->pixelBuffer, size);
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
	  lpm013m126a lpm = data;
		if ( lpm->pixelBuffer )
			c_free(lpm->pixelBuffer);
		c_free(data);
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
