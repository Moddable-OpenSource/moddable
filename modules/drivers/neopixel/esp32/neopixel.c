/*
 * neopixel.c started as a file in the MicroPython ESP32 project.
 * It was modified to work in the Moddable environment.
 * A later revision incorporated vast changes from ideas and
 * concepts gleaned from the FastLED project, https://github.com/FastLED
 */
/*
 * This file is part of the MicroPython ESP32 project, https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 LoBo (https://github.com/loboris)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 FastLED
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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
#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.defines.h"

#include <math.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/rmt.h"

#include "log/include/esp_log.h"

#include "neopixel.h"

static pixel_settings_t *gControllers[MODDEF_NEOPIXEL_STRINGS_MAX];
static pixel_settings_t *gOnChannel[MODDEF_NEOPIXEL_CHANNELS_MAX];

static int gNumControllers = 0;
static int gNumStarted = 0;
static int gNumDone = 0;
static int gNext = 0;

static intr_handle_t gRMT_intr_handle = NULL;
static xSemaphoreHandle gTX_sem = NULL;
static bool gInitialized = false;

void initRMT();
static void interruptHandler(void *arg);
static void startNext(int channel);
static void doneOnChannel(rmt_channel_t channel, void *arg);
void fillHalfRMTBuffer(pixel_settings_t *px);
void startOnChannel(pixel_settings_t *px, int channel);
static void copyPixelData(pixel_settings_t *px);


#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	#define NEOPIXEL_PREPARE_DATA(px) copyPixelData(px)
#else
	#define NEOPIXEL_PREPARE_DATA(px) convertAllPixelData(px)
	static void convertAllPixelData(pixel_settings_t *px);
#endif


#if mxInstrument
	#include "modInstrumentation.h"
	#define countPixels modInstrumentationAdjust
#else
	#define countPixels(what, value)
#endif


#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
// which core to run on
#ifndef MODDEF_NEOPIXEL_CORE
#define MODDEF_NEOPIXEL_CORE	1
#endif

static TaskHandle_t neopixelDisplayTaskHandle = NULL;
static TaskHandle_t userTaskHandle = NULL;

void neopixelDisplayTaskShow() {
	if (gNumStarted == 0) {
		// first one sets everything up
		initRMT();
		xSemaphoreTake(gTX_sem, portMAX_DELAY);
	}

	gNumStarted++;

	// only after the last call to showPixels do we start the work
	if (gNumStarted == gNumControllers) {
		gNext = 0;

		// -- First, fill all the available channels
		int channel = 0;
		while (channel < MODDEF_NEOPIXEL_CHANNELS_MAX && gNext < gNumControllers) {
			startNext(channel);
			channel++;
		}

		// wait until data is sent. Interrupt handler will refill
		// the RMT until all is sent, then it returns the semaphore
		xSemaphoreTake(gTX_sem, portMAX_DELAY);
		xSemaphoreGive(gTX_sem);

		gNumStarted = 0;
		gNumDone = 0;
	}

}

void neopixelDisplayTask(void *pvParameters) {
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		neopixelDisplayTaskShow();
		xTaskNotifyGive(userTaskHandle);
	}
}

void puntToNeopixelDisplayTask() {
	if (userTaskHandle == NULL) {
		userTaskHandle = xTaskGetCurrentTaskHandle();
		xTaskNotifyGive(neopixelDisplayTaskHandle);

		const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200);
		ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
		userTaskHandle = 0;
	}
}
#endif


void neopixel_deinit(pixel_settings_t *px) {
	xSemaphoreTake(gTX_sem, portMAX_DELAY);
	vSemaphoreDelete(gTX_sem);
	gTX_sem = NULL;

#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	rmt_set_tx_intr_en(px->rmtChannel, false);

	// kill task if none left 
	vTaskDelete(neopixelDisplayTaskHandle);
	neopixelDisplayTaskHandle = NULL;
#else
	rmt_driver_uninstall(px->rmtChannel);
#endif

	if (px->rmtPixelData) {
		c_free(px->rmtPixelData);
		px->rmtPixelData = NULL;
	}

	gInitialized = false;
}

int neopixel_init(pixel_settings_t *px) {
#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	if (!neopixelDisplayTaskHandle) {
		xTaskCreatePinnedToCore(neopixelDisplayTask, "neopixelDisplayTask", 2048, NULL, 2, &neopixelDisplayTaskHandle, MODDEF_NEOPIXEL_CORE);
	}
#endif

	// T1H
	px->rmtOne.level0 = 1;
	px->rmtOne.duration0 = px->timings.mark.duration0;
	// T1L
	px->rmtOne.level1 = 0;
	px->rmtOne.duration1 = px->timings.mark.duration1;
	// T0H
	px->rmtZero.level0 = 1;
	px->rmtZero.duration0 = px->timings.space.duration0;
	// T0L
	px->rmtZero.level1 = 0;
	px->rmtZero.duration1 = px->timings.space.duration1;

	gControllers[gNumControllers++] = px;
}

static IRAM_ATTR void interruptHandler(void *arg) {
	uint32_t intr_st = RMT.int_st.val;
	uint8_t channel;

	for (channel=0; channel<MODDEF_NEOPIXEL_CHANNELS_MAX; channel++) {
		if (gOnChannel[channel] != NULL) {
			pixel_settings_t *px = gOnChannel[channel];
			int tx_done_bit = channel * (px->nbits/8);
			int tx_next_bit = channel + (px->nbits);

			if (intr_st & BIT(tx_next_bit)) {
				RMT.int_clr.val |= BIT(tx_next_bit);
				fillHalfRMTBuffer(px);
			}
			else {
				if (intr_st & BIT(tx_done_bit)) {
					RMT.int_clr.val |= BIT(tx_done_bit);
					doneOnChannel((rmt_channel_t)channel, 0);
				}
			}
		}
	}
}

void initRMT() {
	if (gInitialized) return;
	for (int i=0; i<MODDEF_NEOPIXEL_CHANNELS_MAX; i++) {
		gOnChannel[i] = NULL;

		// RMT transmit configuration
		rmt_config_t rmt_tx = {};
		rmt_tx.channel = (rmt_channel_t)i;
		rmt_tx.rmt_mode = RMT_MODE_TX;
		rmt_tx.gpio_num = 0;
		rmt_tx.mem_block_num = 1;
		rmt_tx.clk_div = DIVIDER;
		rmt_tx.tx_config.loop_en = false;
		rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
		rmt_tx.tx_config.carrier_en = false;
		rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
		rmt_tx.tx_config.idle_output_en = true;

		rmt_config(&rmt_tx);

#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
		rmt_set_tx_thr_intr_en((rmt_channel_t)i, true, 32);
#else
		rmt_driver_install((rmt_channel_t)i, 0, 0);
#endif
	}

	if (gTX_sem == NULL) {
		gTX_sem = xSemaphoreCreateBinary();
		xSemaphoreGive(gTX_sem);
	}

#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	if (gRMT_intr_handle == NULL)
		esp_intr_alloc(ETS_RMT_INTR_SOURCE, 0, interruptHandler, 0, &gRMT_intr_handle);
#endif

	gInitialized = true;
}

void np_show(pixel_settings_t *px) {
	countPixels(PixelsDrawn, px->pixel_count);
#if MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	NEOPIXEL_PREPARE_DATA(px);
	puntToNeopixelDisplayTask();
#else
	if (gNumStarted == 0) {
		initRMT();
		xSemaphoreTake(gTX_sem, portMAX_DELAY);
	}

	NEOPIXEL_PREPARE_DATA(px);

	gNumStarted++;

	if (gNumStarted == gNumControllers) {
		gNext = 0;

		// -- First, fill all the available channels
		int channel = 0;
		while (channel < MODDEF_NEOPIXEL_CHANNELS_MAX && gNext < gNumControllers)
			startNext(channel++);

		// wait until data is sent. Interrupt handler will refill
		// the RMT until all is sent, then it returns the semaphore
		xSemaphoreTake(gTX_sem, portMAX_DELAY);
		xSemaphoreGive(gTX_sem);

		gNumStarted = 0;
		gNumDone = 0;
		gNext = 0;
	}
#endif
}

#if ! MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
void convertByte(pixel_settings_t *px, uint32_t byteval) {
	// -- Write one byte's worth of RMT pulses to the big buffer
	byteval <<= 24;
	for (register uint32_t j = 0; j < 8; j++) {
		px->rmtBuffer[px->curPulse] = (byteval & 0x80000000L) ? px->rmtOne : px->rmtZero;
		byteval <<= 1;
		px->curPulse++;
	}
}

void convertAllPixelData(pixel_settings_t *px) {
	px->rmtBufferSize = px->pixel_count * px->nbits;
	if (px->rmtBuffer == NULL)
		px->rmtBuffer = (rmt_item32_t *)calloc(px->rmtBufferSize, sizeof(rmt_item32_t));

	px->curPulse = 0;
	int cur = 0;
	uint8_t *loc = px->pixels;
	uint32_t byteval;
	while (cur < px->pixel_count) {
		byteval = (*loc++ * px->brightness) / 255;
		convertByte(px, byteval);
		byteval = (*loc++ * px->brightness) / 255;
		convertByte(px, byteval);
		byteval = (*loc++ * px->brightness) / 255;
		convertByte(px, byteval);
		if (px->nbits == 32) {
			byteval = (*loc++ * px->brightness) / 255;
			convertByte(px, byteval);
		}
		cur++;
	}

	px->rmtBuffer[px->curPulse-1].duration1 = px->timings.reset.duration0;
}
#else
void copyPixelData(pixel_settings_t *px) {
	int cur = 0, pix = 0;
	int size_needed = px->pixel_count * (px->nbits / 8);

	if (size_needed > px->rmtPixelDataSize) {
		if (px->rmtPixelData != NULL)
			c_free(px->rmtPixelData);
		px->rmtPixelDataSize = size_needed;
		px->rmtPixelData = (uint8_t*)c_malloc(px->rmtPixelDataSize);
	}

	while (cur < size_needed) {
		uint32_t byteval = px->pixels[cur];
		byteval = (byteval * px->brightness) / 255;
		px->rmtPixelData[cur++] = byteval & 0xff;
	}
}

#endif

void startNext(int channel) {
	if (gNext < gNumControllers) {
		pixel_settings_t *px = gControllers[gNext];
		startOnChannel(px, channel);
		gNext++;
	}
}

void startOnChannel(pixel_settings_t *px, int channel) {
	px->rmtChannel = (rmt_channel_t)channel;
	gOnChannel[channel] = px;

	// set the pin (on demand)
	rmt_set_gpio(px->rmtChannel, RMT_MODE_TX, px->pin, false);

#if ! MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	rmt_register_tx_end_callback(doneOnChannel, 0);
	rmt_write_items(px->rmtChannel, px->rmtBuffer, px->rmtBufferSize, false);
#else
	px->curPulse = 0;
	px->curByte = 0;

	fillHalfRMTBuffer(px);
	fillHalfRMTBuffer(px);

	// turn on interrupts
	rmt_set_tx_intr_en(px->rmtChannel, true);

	// start it up
	rmt_tx_start(px->rmtChannel, true);
#endif
}

void IRAM_ATTR doneOnChannel(rmt_channel_t channel, void *arg) {
	pixel_settings_t *px = gOnChannel[channel];
	portBASE_TYPE HPTaskAwoken = 0;

	gOnChannel[channel] = NULL;
	gNumDone++;

	if (gNumDone == gNumControllers) {
		xSemaphoreGiveFromISR(gTX_sem, &HPTaskAwoken);
		if (HPTaskAwoken == pdTRUE)
			portYIELD_FROM_ISR();
	}
	else {
		if (gNext < gNumControllers)
			startNext(channel);
	}
}

void IRAM_ATTR fillHalfRMTBuffer(pixel_settings_t *px) {
	uint32_t one_val = px->rmtOne.val;
	uint32_t zero_val = px->rmtZero.val;

	int pulses = 0;
	uint8_t byteval;
	register uint32_t j;

	while ((pulses < 32) && (px->curByte < px->rmtPixelDataSize)) {
		byteval = px->rmtPixelData[px->curByte++];

		for (j=0; j<8; j++) {
			uint32_t val = (byteval & 0x80L) ? one_val : zero_val;
			RMTMEM.chan[px->rmtChannel].data32[px->curPulse].val = val;
			byteval <<= 1;
			px->curPulse++;
		}
		pulses += 8;
	}

	if (px->curByte == px->rmtPixelDataSize) {
		while (pulses < 32) {
			RMTMEM.chan[px->rmtChannel].data32[px->curPulse].val = 0;
			px->curPulse++;
			pulses++;
		}
	}

	if (px->curPulse >= 64)
		px->curPulse = 0;
}


// Get color value of RGB component
//---------------------------------------------------
static uint8_t offset_color(char o, uint32_t color) {
	uint8_t clr = 0;
	switch(o) {
		case 'R':
			clr = (uint8_t)(color >> 24);
			break;
		case 'G':
			clr = (uint8_t)(color >> 16);
			break;
		case 'B':
			clr = (uint8_t)(color >> 8);
			break;
		case 'W':
			clr = (uint8_t)(color & 0xFF);
			break;
		default:
			clr = 0;
	}
	return clr;
}

// Set pixel color at buffer position from RGB color value
//=========================================================================
void np_set_pixel_color(pixel_settings_t *px, uint16_t idx, uint32_t color)
{
	uint16_t ofs = idx * (px->nbits / 8);
	px->pixels[ofs] = offset_color(px->color_order[0], color);
	px->pixels[ofs+1] = offset_color(px->color_order[1], color);
	px->pixels[ofs+2] = offset_color(px->color_order[2], color);
	if (px->nbits == 32) px->pixels[ofs+3] = offset_color(px->color_order[3], color);
}

// Set pixel color at buffer position from HSB color value
//============================================================================================================
void np_set_pixel_color_hsb(pixel_settings_t *px, uint16_t idx, float hue, float saturation, float brightness)
{
	uint32_t color = hsb_to_rgb(hue, saturation, brightness);
	np_set_pixel_color(px, idx, color);
}

// Get RGB color value from RGB components corrected by brightness factor
//=============================================================================
uint32_t np_get_pixel_color(pixel_settings_t *px, uint16_t idx, uint8_t *white)
{
	uint32_t clr = 0;
	uint32_t color = 0;
	uint8_t bpp = px->nbits/8;
	uint16_t ofs = idx * bpp;

	for (int i=0; i < bpp; i++) {
		clr = (uint16_t)px->pixels[ofs+i];
		switch(px->color_order[i]) {
			case 'R':
				color |= (uint32_t)clr << 16;
				break;
			case 'G':
				color |= (uint32_t)clr << 8;
				break;
			case 'B':
				color |= (uint32_t)clr;
				break;
			case 'W':
				*white = px->pixels[ofs+i];
				break;
		}
	}
	return color;
}


// Clear the Neopixel color buffer
//=================================
void np_clear(pixel_settings_t *px)
{
	memset(px->pixels, 0, px->pixel_count * (px->nbits/8));
}

//------------------------------------
static float Min(double a, double b) {
	return a <= b ? a : b;
}

//------------------------------------
static float Max(double a, double b) {
	return a >= b ? a : b;
}

// Convert 24-bit color to HSB representation
//===================================================================
void rgb_to_hsb( uint32_t color, float *hue, float *sat, float *bri )
{
	float delta, min;
	float h = 0, s, v;
	uint8_t red = (color >> 16) & 0xFF;
	uint8_t green = (color >> 8) & 0xFF;
	uint8_t blue = color & 0xFF;

	min = Min(Min(red, green), blue);
	v = Max(Max(red, green), blue);
	delta = v - min;

	if (v == 0.0) s = 0;
	else s = delta / v;

	if (s == 0)	h = 0.0;
	else
	{
		if (red == v)
			h = (green - blue) / delta;
		else if (green == v)
			h = 2 + (blue - red) / delta;
		else if (blue == v)
			h = 4 + (red - green) / delta;

		h *= 60;

		if (h < 0.0) h = h + 360;
	}

	*hue = h;
	*sat = s;
	*bri = v / 255;
}

// Convert HSB color to 24-bit color representation
//============================================================
uint32_t hsb_to_rgb(float _hue, float _sat, float _brightness)
{
	float red = 0.0;
	float green = 0.0;
	float blue = 0.0;

	if (_sat == 0.0) {
		red = _brightness;
		green = _brightness;
		blue = _brightness;
	}
	else {
		if (_hue >= 360.0) _hue = fmod(_hue, 360);

		int slice = (int)(_hue / 60.0);
		float hue_frac = (_hue / 60.0) - slice;

		float aa = _brightness * (1.0 - _sat);
		float bb = _brightness * (1.0 - _sat * hue_frac);
		float cc = _brightness * (1.0 - _sat * (1.0 - hue_frac));

		switch(slice) {
			case 0:
				red = _brightness;
				green = cc;
				blue = aa;
				break;
			case 1:
				red = bb;
				green = _brightness;
				blue = aa;
				break;
			case 2:
				red = aa;
				green = _brightness;
				blue = cc;
				break;
			case 3:
				red = aa;
				green = bb;
				blue = _brightness;
				break;
			case 4:
				red = cc;
				green = aa;
				blue = _brightness;
				break;
			case 5:
				red = _brightness;
				green = aa;
				blue = bb;
				break;
			default:
				red = 0.0;
				green = 0.0;
				blue = 0.0;
				break;
		}
	}

	return (uint32_t)((uint8_t)(red * 255.0) << 16) | ((uint8_t)(green * 255.0) << 8) | ((uint8_t)(blue * 255.0));
}

// Convert HSB color to 24-bit color representation
// _hue: 0 ~ 359
// _sat: 0 ~ 255
// _bri: 0 ~ 255
//=======================================================
uint32_t hsb_to_rgb_int(int hue, int sat, int brightness)
{
	float _hue = (float)hue;
	float _sat = (float)((float)sat / 1000.0);
	float _brightness = (float)((float)brightness / 1000.0);
	float red = 0.0;
	float green = 0.0;
	float blue = 0.0;

	if (_sat == 0.0) {
		red = _brightness;
		green = _brightness;
		blue = _brightness;
	}
	else {
		if (_hue >= 360.0) _hue = fmod(_hue, 360);

		int slice = (int)(_hue / 60.0);
		float hue_frac = (_hue / 60.0) - slice;

		float aa = _brightness * (1.0 - _sat);
		float bb = _brightness * (1.0 - _sat * hue_frac);
		float cc = _brightness * (1.0 - _sat * (1.0 - hue_frac));

		switch(slice) {
			case 0:
				red = _brightness;
				green = cc;
				blue = aa;
				break;
			case 1:
				red = bb;
				green = _brightness;
				blue = aa;
				break;
			case 2:
				red = aa;
				green = _brightness;
				blue = cc;
				break;
			case 3:
				red = aa;
				green = bb;
				blue = _brightness;
				break;
			case 4:
				red = cc;
				green = aa;
				blue = _brightness;
				break;
			case 5:
				red = _brightness;
				green = aa;
				blue = bb;
				break;
			default:
				red = 0.0;
				green = 0.0;
				blue = 0.0;
				break;
		}
	}

	return (uint32_t)((uint8_t)(red * 255.0) << 16) | ((uint8_t)(green * 255.0) << 8) | ((uint8_t)(blue * 255.0));
}

