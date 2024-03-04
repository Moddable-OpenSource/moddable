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

#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/rmt_tx.h"

#define RMT_LED_STRIP_RESOLUTION_HZ	10000000

#define NEOPIXEL_MSG_IDLE	0
#define NEOPIXEL_MSG_STOP	1
#define NEOPIXEL_MSG_DRAW	2

typedef struct pixel_settings {
	uint8_t 		*pixels;		// buffer containing pixel values, 3 (RGB) or 4 (RGBW) bytes per pixel
	uint8_t 		*modPixels;		// buffer containing modified (brightness) pixel values, used during encode
	uint16_t		pixel_count;	// number of used pixels
	uint16_t		byte_count;		// precalculated pixels bytecount
	uint8_t			brightness;		// brightness factor applied to pixel color
	uint8_t			pin;
	char			color_order[5];
	uint8_t			nbits;			// number of bits used (24 for RGB devices, 32 for RGBW devices)
	rmt_encoder_handle_t	led_encoder;
	rmt_channel_handle_t	rmtChannel;

	uint8_t			initialized;

	QueueHandle_t		neopixel_msgQueue;
	TaskHandle_t		neopixel_task;
	TaskHandle_t		caller_task;
} pixel_settings_t;

void np_set_pixel_color(pixel_settings_t *px, uint16_t idx, uint32_t color);
void np_set_pixel_color_hsb(pixel_settings_t *px, uint16_t idx, float hue, float saturation, float brightness);
uint32_t np_get_pixel_color(pixel_settings_t *px, uint16_t idx, uint8_t *white);
void np_show(pixel_settings_t *px);
void np_clear(pixel_settings_t *px);

int neopixel_init(pixel_settings_t *px);
void neopixel_deinit(pixel_settings_t *px);

void rgb_to_hsb( uint32_t color, float *hue, float *sat, float *bri );
uint32_t hsb_to_rgb(float hue, float saturation, float brightness);
uint32_t hsb_to_rgb_int(int hue, int sat, int brightness);
