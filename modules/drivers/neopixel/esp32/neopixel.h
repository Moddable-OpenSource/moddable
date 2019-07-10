/*
 * neopixel.h started as a file in the MicroPython ESP32 project.
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

#include "driver/gpio.h"
#include "driver/rmt.h"

#ifndef MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
    #define MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER   0
#endif

// maximum number of neopixel strings - 32 is okay
#ifndef MODDEF_NEOPIXEL_STRINGS_MAX
#define MODDEF_NEOPIXEL_STRINGS_MAX 1
#endif

// number of RMT channels to use - was 8
#ifndef MODDEF_NEOPIXEL_CHANNELS_MAX
#define MODDEF_NEOPIXEL_CHANNELS_MAX 1
#endif


#define DIVIDER                4	// 80 MHz clock divider
#define RMT_DURATION_NS     12.5	// minimum time of a single RMT duration based on 80 MHz clock (ns)
#define RMT_PERIOD_NS         50	// minimum bit time based on 80 MHz clock and divider of 4

#define F_CPU_MHZ CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
#define NS(_NS) (((_NS * F_CPU_MHZ) + 999) / 1000)

#define ESPCLKS_TO_NS(_CLKS) (((long)(_CLKS) * 1000L) / F_CPU_MHZ)
#define F_CPU_RMT   (  80000000L)
#define NS_PER_SEC  (1000000000L)
#define CYCLES_PER_SEC  (F_CPU_RMT/DIVIDER)
#define NS_PER_CYCLE    (NS_PER_SEC / CYCLES_PER_SEC)
#define NS_TO_CYCLES(n) ((n) / NS_PER_CYCLE)

#define TO_RMT_CYCLES(_CLKS) NS_TO_CYCLES(ESPCLKS_TO_NS(_CLKS))

#define RMT_RESET_DURATION  NS_TO_CYCLES(50000)



typedef struct bit_timing {
	uint8_t level0;
	uint16_t duration0;
	uint8_t level1;
	uint16_t duration1;
} bit_timing_t;

typedef struct pixel_timing {
	bit_timing_t mark;
	bit_timing_t space;
	bit_timing_t reset;
} pixel_timing_t;

typedef struct pixel_settings {
	uint8_t 		*pixels;		// buffer containing pixel values, 3 (RGB) or 4 (RGBW) bytes per pixel
	pixel_timing_t	timings;	// timing data from which the pixels BIT data are formed
	uint16_t		pixel_count;	// number of used pixels
	uint8_t			brightness;		// brightness factor applied to pixel color
	uint8_t			pin;
	char			color_order[5];
	uint8_t			nbits;			// number of bits used (24 for RGB devices, 32 for RGBW devices)

	rmt_channel_t	rmtChannel;
    rmt_item32_t    rmtZero;
    rmt_item32_t    rmtOne;

	uint8_t			*rmtPixelData;
	int				rmtPixelDataSize;
	int				curByte;
	uint16_t		curPulse;

#if ! MODDEF_NEOPIXEL_CUSTOM_RMT_DRIVER
	rmt_item32_t	*rmtBuffer;
	int				rmtBufferSize;
#else
#endif

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
