/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// https://raw.githubusercontent.com/loboris/MicroPython_ESP32_psRAM_LoBo/master/MicroPython_BUILD/components/micropython/esp32/libs/ow/owb_rmt.h

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h" // for xsID_ values
#include "mc.defines.h"

//#include "owb.h"
//#include "owb_rmt.h"

// See example here:
// https://github.com/DavidAntliff/esp32-ds18b20-example/blob/master/main/app_main.c#L53

typedef struct
{
  xsMachine *the;
  xsSlot obj;
  uint8_t pin;
  uint8_t val;
} modOneWireRecord, *modOneWire;

void xs_onewire_destructor(void *data)
{
  modOneWire onewire = data;
  if (NULL == onewire)
    return;
  c_free(onewire);
}

void xs_onewire(xsMachine *the)
{
  modOneWire onewire;
  int pin;

  xsmcVars(1);

  if (!xsmcHas(xsArg(0), xsID_pin))
    xsUnknownError("pin missing");

  onewire = c_malloc(sizeof(modOneWireRecord));
  if (!onewire)
    xsUnknownError("no memory");

  xsmcGet(xsVar(0), xsArg(0), xsID_pin);
  pin = xsmcToInteger(xsVar(0));

  onewire->the = the;
  onewire->obj = xsThis;
  onewire->pin = pin;
  onewire->val = 100;

  // owb_rmt_initialize

  xsRemember(onewire->obj);

  xsmcSetHostData(xsThis, onewire);
}

void xs_onewire_close(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  xsForget(onewire->obj);
  xs_onewire_destructor(onewire);
  xsmcSetHostData(xsThis, NULL);
}

void xs_onewire_read(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);

  int argc = xsmcToInteger(xsArgc);
  if (argc == 0)
  {

    int count = 1;
    xsmcSetInteger(xsResult, count);
  }
  else
  {
    int count = xsmcToInteger(xsArg(0));
    xsResult = xsArrayBuffer(NULL, count);
    uint8_t *buffer = xsmcToArrayBuffer(xsResult);
    buffer[1] = 66;
    buffer[2] = 67;
    //size = fread(xsToArrayBuffer(xsResult), 1, size, file);
  }
}

void xs_onewire_write(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  int value = xsmcToInteger(xsArg(0));
  if (value < 0)
    value = 0;
  else if (value > 255)
    value = 255;
  onewire->val = value;
}

void xs_onewire_select(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  xsTrace("xs_onewire_select: ");
  //xsTrace(xsmcToString(xsArg(0)));xsTrace("\n");
}

void xs_onewire_search(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);

  xsmcVars(2);
  xsVar(0) = xsNewArray(0);

  uint8_t buffer[] = {0x28, 0xCA, 0x00, 0xA9, 0x04, 0x00, 0x00, 0xEA};

  xsCall1(xsVar(0), xsID_push, xsArrayBuffer(buffer, 8));
  buffer[6] = 0x78;
  buffer[3] = 0x92;
  xsCall1(xsVar(0), xsID_push, xsArrayBuffer(buffer, 8));

  xsResult = xsVar(0);
}

void xs_onewire_reset(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  onewire->val = 0;
  xsmcSetInteger(xsResult, 1);
}

void xs_onewire_crc(xsMachine *the)
{
  uint8_t crc = 0;
  modOneWire onewire = xsmcGetHostData(xsThis);

  xsmcSetInteger(xsResult, ++crc);
}