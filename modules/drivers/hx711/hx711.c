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
#include "xsPlatform.h"
#include "xsmc.h"
#include "modGPIO.h"
#include "mc.xs.h"      // for xsID_ values
#include "hx711.h"

uint8_t xs_HX711_shftin(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin) {
    uint8_t value = 0;
    uint8_t i;
    for (i = 0; i < 8; ++i) {
        modGPIOWrite(clk_pin, 1);
        modDelayMicroseconds(1);
        value |= modGPIORead(dat_pin) << (7 - i);
        modGPIOWrite(clk_pin, 0);
        modDelayMicroseconds(1);
    }
    return value;
}

void xs_HX711_destructor(void *data) {
	if (data) {
		HX711 hx711 = data;

		modGPIOUninit(&hx711->clk_pin);
		modGPIOUninit(&hx711->dat_pin);
		c_free(data);
  }
}

bool xs_HX711_is_ready (modGPIOConfigurationRecord *dat_pin) {
  uint8_t result = modGPIORead(dat_pin);
  return result == 0;
}

uint8_t xs_HX711_wait_ready (modGPIOConfigurationRecord *dat_pin) {
  uint16_t seq = 0;
  while(!xs_HX711_is_ready(dat_pin)) {
    modDelayMilliseconds(1);
    if (seq++ > 1000) {
      return 1;
    }
  }
  return 0;
}

long xs_HX711_read(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin) {
  if (xs_HX711_wait_ready(dat_pin) > 0) {
    return -1;
  }

  unsigned long value = 0;
  uint8_t data[3] = {0};
  uint8_t filter = 0x00;

  modCriticalSectionBegin();
  data[2] = xs_HX711_shftin(dat_pin, clk_pin);
  data[1] = xs_HX711_shftin(dat_pin, clk_pin);
  data[0] = xs_HX711_shftin(dat_pin, clk_pin);

  for (unsigned int i = 0; i < MODDEF_HX711_GAIN; i++) {
    modGPIOWrite(clk_pin, 1);
    modDelayMicroseconds(1);
    modGPIOWrite(clk_pin, 0);
    modDelayMicroseconds(1);
  }
  modCriticalSectionEnd();

  if (data[2] & 0x80) {
    filter = 0xFF;
  } else {
    filter = 0x00;
  }

  value = (filter << 24 | data[2] << 16 | data[1] << 8 | data[0]);

  return value;
}

long xs_HX711_read_average(modGPIOConfigurationRecord *dat_pin, modGPIOConfigurationRecord *clk_pin, int times) {
  long sum = 0;
  for (int i = 0; i < times; i++) {
    long v = xs_HX711_read(dat_pin, clk_pin);
    if (v < 0) {
      return -1;
    }
    sum += v;
  }
  return sum / times;
}

void xs_HX711_get_raw_value(xsMachine *the) {
  int times = 3;
  HX711 hx711 = xsmcGetHostData(xsThis);
  long val = xs_HX711_read_average(&hx711->dat_pin, &hx711->clk_pin, times);
  if (val < 0) {
    xsUnknownError("read failed");
  }
  xsmcSetNumber(xsResult, val);
}

void xs_HX711_get_scale(xsMachine *the){
  HX711 hx711 = xsmcGetHostData(xsThis);
  xsmcSetNumber(xsResult, hx711->scale);
}

void xs_HX711_set_scale(xsMachine *the){
  float scale = xsmcToNumber(xsArg(0));
  HX711 hx711 = xsmcGetHostData(xsThis);
  hx711->scale = scale;
}

void xs_HX711_get_offset(xsMachine *the){
  HX711 hx711 = xsmcGetHostData(xsThis);
  xsmcSetNumber(xsResult, hx711->offset);
}

void xs_HX711_get_dat_pin(xsMachine *the){
  HX711 hx711 = xsmcGetHostData(xsThis);
  xsmcSetNumber(xsResult, hx711->dat_pin.pin);
}

void xs_HX711_get_clk_pin(xsMachine *the){
  HX711 hx711 = xsmcGetHostData(xsThis);
  xsmcSetNumber(xsResult, hx711->clk_pin.pin);
}

void xs_HX711_set_offset(xsMachine *the){
  long offset = xsmcToInteger(xsArg(0));
  HX711 hx711 = xsmcGetHostData(xsThis);
  hx711->offset = offset;
}

void xs_HX711_get_value(xsMachine *the) {
  HX711 hx711 = xsmcGetHostData(xsThis);
  int times = 3;
  long val = xs_HX711_read_average(&hx711->dat_pin, &hx711->clk_pin, times);
  if (val < 0) {
    xsUnknownError("read failed");
  }
  val = (val - hx711->offset) * MODDEF_HX711_ADC1bit / hx711->scale;
  xsmcSetNumber(xsResult, val);
}

void xs_HX711(xsMachine *the) {
  HX711 hx711;
  hx711 = calloc(1, sizeof(HX711Record));
	if (!hx711)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, hx711);

  int clk_pin;
  int dat_pin;
  float scale;
  if (xsmcArgc > 0) {
    xsmcVars(3);
    xsmcGet(xsVar(0), xsArg(0), xsID_clk);
    xsmcGet(xsVar(1), xsArg(0), xsID_dat);
    xsmcGet(xsVar(2), xsArg(0), xsID_scale);

    clk_pin = xsmcTest(xsVar(0)) ? xsmcToInteger(xsVar(0)) : MODDEF_HX711_CLK_PIN;
    dat_pin = xsmcTest(xsVar(1)) ? xsmcToInteger(xsVar(1)) : MODDEF_HX711_DAT_PIN;
    scale = xsmcTest(xsVar(2)) ? xsmcToNumber(xsVar(2)) : MODDEF_HX711_SCALE;
  } else {
    clk_pin = MODDEF_HX711_CLK_PIN;
    dat_pin = MODDEF_HX711_DAT_PIN;
    scale = MODDEF_HX711_SCALE;
  }

  char s[256];
  sprintf(s, "scale: %f", scale);
  xsTrace(s);

	if (modGPIOInit(&hx711->clk_pin, NULL, clk_pin, kModGPIOOutput))
		xsUnknownError("can't init clk pin");
	if (modGPIOInit(&hx711->dat_pin, NULL, dat_pin, kModGPIOInput))
		xsUnknownError("can't init dat pin");

  modGPIOWrite(&hx711->clk_pin, 1);
  modDelayMicroseconds(100);
  modGPIOWrite(&hx711->clk_pin, 0);

  hx711->offset = xs_HX711_read_average(&hx711->dat_pin, &hx711->clk_pin, 3);
  hx711->scale = scale;
}
