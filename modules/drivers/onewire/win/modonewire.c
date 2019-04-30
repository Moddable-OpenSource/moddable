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

    /* Example reads to mock DS18s20
0 28CC2E230500006B:24.1875 read 9: 800115097FFF10107D
1 283260DC04000001:24 read 9: 800115097FFF10107D
0 28CC2E230500006B:24.375 read 9: 7A0115097FFF0610B0
1 283260DC04000001:23.625 read 9: 860115097FFF0A10E5
0 28CC2E230500006B:24.375 read 9: 7A0115097FFF0610B0
1 283260DC04000001:23.625 read 9: 860115097FFF0A10E5
0 28CC2E230500006B:24.375 read 9: 790115097FFF0710B1
1 283260DC04000001:23.5625 read 9: 850115097FFF0B10E4
0 28CC2E230500006B:24.3125 read 9: 7A0115097FFF0610B0
*/
    int count = xsmcToInteger(xsArg(0));
    xsResult = xsArrayBuffer(NULL, count);
    uint8_t *buffer = xsmcToArrayBuffer(xsResult);

  // hack as 24.1875c ->  800115097FFF10107D
    buffer[0] = 0x80;
    buffer[1] = 0x01;
    buffer[2] = 0x15;
    buffer[3] = 0x09;
    buffer[4] = 0x7F;
    buffer[5] = 0xFF;
    buffer[6] = 0x10;
    buffer[7] = 0x10;
    buffer[8] = 0x7D; // Calc as CRC ?
  }
}

void xs_onewire_write(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  uint8_t value = xsmcToInteger(xsArg(0));
  if ((value < 0) || (value > 255))
    xsRangeError("bad value");
  onewire->val = value;
}

void xs_onewire_select(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
}

void xs_onewire_search(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);


  xsmcVars(1);
  xsResult = xsNewArray(0);

  uint8_t buffer[] = {0x28, 0xCA, 0x00, 0xA9, 0x04, 0x00, 0x00, 0xEA};
  xsCall1(xsResult, xsID_push, xsArrayBuffer(buffer, 8));

  buffer[6] = 0x78;
  buffer[3] = 0x92;
  xsCall1(xsResult, xsID_push, xsArrayBuffer(buffer, 8));
}

void xs_onewire_search2(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);

  //xsmcVars(2);
  //xsVar(0) = xsNewArray(0);

  uint8_t buffer[] = {0x28, 0xCA, 0x00, 0xA9, 0x04, 0x00, 0x00, 0xEA};

  xsmcVars(1);
  xsResult = xsNewArray(0);
  xsCall1(xsResult, xsID_push, xsArrayBuffer(buffer, 8));
  buffer[6] = 0x78;
  buffer[3] = 0x92;
  xsCall1(xsResult, xsID_push, xsArrayBuffer(buffer, 8));

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