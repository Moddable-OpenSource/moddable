/*
 *   Copyright (c) 2020 Shinya Ishikawa
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
#ifndef __MODHX711_H__
#define __MODHX711_H__

typedef struct {
	modGPIOConfigurationRecord dat_pin;
	modGPIOConfigurationRecord clk_pin;
	float scale;
	long offset;
} HX711Record, *HX711;

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"      // for xsID_ values
#include "hx711.h"

#ifndef MODDEF_HX711_OUT_VOL
  #define MODDEF_HX711_OUT_VOL  0.001f
#endif
#ifndef MODDEF_HX711_LOAD
  #define MODDEF_HX711_LOAD  2000.0f
#endif
#ifndef MODDEF_HX711_DAT_PIN
  #define MODDEF_HX711_DAT_PIN (36)
#endif
#ifndef MODDEF_HX711_CLK_PIN
  #define MODDEF_HX711_CLK_PIN (26)
#endif
#ifndef MODDEF_HX711_GAIN
  #define MODDEF_HX711_GAIN (1)
#endif
#ifndef MODDEF_HX711_R1
  #define MODDEF_HX711_R1 20000.0f
#endif
#ifndef MODDEF_HX711_R2
  #define MODDEF_HX711_R2 8200.0f
#endif
#ifndef MODDEF_HX711_VBG
  #define MODDEF_HX711_VBG 1.25f
#endif
#ifndef MODDEF_HX711_AVDD
  #define MODDEF_HX711_AVDD      (MODDEF_HX711_VBG*((MODDEF_HX711_R1+MODDEF_HX711_R2)/MODDEF_HX711_R2))
#endif
#ifndef MODDEF_HX711_ADC1bit
  #define MODDEF_HX711_ADC1bit   (MODDEF_HX711_AVDD/16777216) //16777216==(2^24)
#endif
#ifndef MODDEF_HX711_PGA
  #define MODDEF_HX711_PGA 128
#endif
#ifndef MODDEF_HX711_SCALE
  #define MODDEF_HX711_SCALE     (MODDEF_HX711_OUT_VOL * MODDEF_HX711_AVDD / MODDEF_HX711_LOAD *MODDEF_HX711_PGA)
#endif

void xs_HX711(xsMachine *the);
void xs_HX711_destructor(void *data);
void xs_HX711_get_value(xsMachine *the);
void xs_HX711_get_raw_value(xsMachine *the);
void xs_HX711_get_dat_pin(xsMachine *the);
void xs_HX711_get_clk_pin(xsMachine *the);
void xs_HX711_get_offset(xsMachine *the);
void xs_HX711_set_offset(xsMachine *the);
void xs_HX711_get_scale(xsMachine *the);
void xs_HX711_set_scale(xsMachine *the);

// private methods
uint8_t xs_HX711_shftin(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin);
bool xs_HX711_is_ready (modGPIOConfigurationRecord *dat_pin);
uint8_t xs_HX711_wait_ready (modGPIOConfigurationRecord *dat_pin);
long xs_HX711_read(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin);
long xs_HX711_read_average(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin, int times);

#endif