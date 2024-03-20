// Copyright © 2023 by Thorsten von Eicken.

#include "xsPlatform.h"
#include "xsmc.h"
#include "modGPIO.h"
#include "mc.xs.h" // for xsID_* constants

#define xsmcVar(x) xsVar(x)

typedef struct {
  modGPIOConfigurationRecord clk;
  modGPIOConfigurationRecord din;
  int gain;
} hx711_data;

void xs_HX711_init(xsMachine *the) {
  if (xsmcArgc != 3) xsUnknownError("invalid arguments");
  int clk_pin = xsmcToInteger(xsArg(0));
  int din_pin = xsmcToInteger(xsArg(1));

  hx711_data *data = c_malloc(sizeof(hx711_data));
  if (data == NULL) xsUnknownError("can't allocate data");
  data->gain = xsmcToInteger(xsArg(2));
  if (modGPIOInit(&data->clk, NULL, clk_pin, kModGPIOOutput))
    xsUnknownError("can't init clk pin");
  modGPIOWrite(&data->clk, 0);
  if (modGPIOInit(&data->din, NULL, din_pin, kModGPIOInput))
    xsUnknownError("can't init dat pin");

  xsmcSetHostData(xsThis, data);
}

void xs_HX711_destructor(void *hostData) {
  hx711_data *data = hostData;
  modGPIOUninit(&data->clk);
  modGPIOUninit(&data->din);
  c_free(data);
}

void xs_HX711_readable(xsMachine *the) {
  hx711_data *data = xsmcGetHostData(xsThis);
  xsmcSetBoolean(xsResult, modGPIORead(&data->din) == 0);
}

void xs_HX711_read(xsMachine *the) {
  hx711_data *data = xsmcGetHostData(xsThis);

  // check data is ready
  if (modGPIORead(&data->din) != 0) {
    xsmcSetUndefined(xsResult);
    return;
  }
  modCriticalSectionBegin();

  // read 24 bits
  int32_t value = 0;
  for (int i = 0; i < 24; i++) {
    modGPIOWrite(&data->clk, 1);
    modDelayMicroseconds(1);
    modGPIOWrite(&data->clk, 0);
    value = (value<<1) | (modGPIORead(&data->din) & 1);
  }

  // sign-extend 24->32 bits
  value = (value << 8) >> 8;

  // signal gain
  for (int i = 0; i < data->gain; i++) {
    modGPIOWrite(&data->clk, 1);
    modGPIOWrite(&data->clk, 0);
  }
  modCriticalSectionEnd();

  // return value
  xsmcSetInteger(xsResult, value);
}
