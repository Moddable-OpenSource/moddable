/*
 * Copyright (c) 2018-2023  Moddable Tech, Inc.
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
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#include "neopixel.h"

#ifndef MODDEF_NEOPIXEL_CORE
	#define MODDEF_NEOPIXEL_CORE	1
#endif
#define	NP_QUEUE_LEN 2
#define NP_QUEUE_ITEM_SIZE	4

#if mxInstrument
	#include "modInstrumentation.h"
	#define countPixels modInstrumentationAdjust
#else
	#define countPixels(what, value)
#endif

static uint8_t rmt_num;

static void displayTask(void *param);

led_strip_encoder_config_t encoder_config = {
	.resolution = RMT_LED_STRIP_RESOLUTION_HZ
};

rmt_transmit_config_t tx_config = {
	.loop_count = 0,
};

int neopixel_init(pixel_settings_t *px) {
	if (rmt_num == (SOC_RMT_GROUPS * SOC_RMT_TX_CANDIDATES_PER_GROUP))
		return -1;
	rmt_num++;
	px->neopixel_msgQueue = xQueueCreate(NP_QUEUE_LEN, NP_QUEUE_ITEM_SIZE);
	px->caller_task = xTaskGetCurrentTaskHandle();

	xTaskCreatePinnedToCore(displayTask, "neoPixelDispayTask", 2048, px, 2, &px->neopixel_task, MODDEF_NEOPIXEL_CORE);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 	// wait for creation

	return 0;
}

static void displayTask(void *param)
{
	pixel_settings_t *px = (pixel_settings_t*)param;
	bool done = false;

	px->byte_count = px->pixel_count * (px->nbits / 8);
	// RMT transmit configuration
	rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.gpio_num = px->pin,
		.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL,
		.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
		.trans_queue_depth = 4,
		.flags.invert_out = false,
		.flags.with_dma = false,
	};

	ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &px->rmtChannel));
	rmt_enable(px->rmtChannel);
	ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &px->led_encoder));
	px->initialized = true;

	xTaskNotifyGive(px->caller_task);

	while (!done) {
		uint8_t *src = px->pixels;
		uint8_t *dst = px->modPixels;
		int i;
		int msg = 0;
		xQueueReceive(px->neopixel_msgQueue, (void*)&msg, portMAX_DELAY);
		switch (msg) {
			case NEOPIXEL_MSG_STOP:
				done = true;
				break;
			case NEOPIXEL_MSG_DRAW:
				// adjust for brightness
				// could do gamma correction here
				ESP_ERROR_CHECK(rmt_tx_wait_all_done(px->rmtChannel, portMAX_DELAY));
				for (i=0; i<px->byte_count; i++)
					*dst++ = ((*src++) * px->brightness) >> 8;
				countPixels(PixelsDrawn, px->pixel_count);

				xTaskNotifyGive(px->caller_task);
				ESP_ERROR_CHECK(rmt_transmit(px->rmtChannel, px->led_encoder, px->modPixels, px->byte_count, &tx_config));

				break;
		}
	}

	px->initialized = false;
	xTaskNotifyGive(px->caller_task);
    vTaskDelete( NULL );
}

void neopixel_deinit(pixel_settings_t *px) {
	int msg = NEOPIXEL_MSG_STOP;
	if (px->initialized) {
		px->caller_task = xTaskGetCurrentTaskHandle();
		xQueueSend(px->neopixel_msgQueue, &msg, portMAX_DELAY);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 	// wait for task to stop
	
		rmt_disable(px->rmtChannel);
		rmt_del_channel(px->rmtChannel);
		rmt_del_encoder(px->led_encoder);

		vQueueDelete(px->neopixel_msgQueue);

		rmt_num--;
		px->initialized = false;
	}
}

void np_show(pixel_settings_t *px) {
	int msg = NEOPIXEL_MSG_DRAW;
	if (px->initialized) {
		px->caller_task = xTaskGetCurrentTaskHandle();
		xQueueSend(px->neopixel_msgQueue, &msg, portMAX_DELAY);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 	// wait for rmt start
	}
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

