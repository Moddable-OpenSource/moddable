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

typedef struct
{
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

void xs_onewire_reset(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  onewire->val = 0;
  xsmcSetInteger(xsResult, 1);
}

/**
 * @brief 1-Wire 8-bit CRC lookup.
 * @param[in] crc Starting CRC value. Pass in prior CRC to accumulate.
 * @param[in] data Byte to feed into CRC.
 * @return Resultant CRC value.
 */
static uint8_t _calc_crc(uint8_t crc, uint8_t data)
{
  // https://www.maximintegrated.com/en/app-notes/index.mvp/id/27
  static const uint8_t table[256] = {
      0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
      157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
      35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
      190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
      70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
      219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
      101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
      248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
      140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
      17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
      175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
      50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
      202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
      87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
      233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
      116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

  return table[crc ^ data];
}

static uint8_t _calc_crc_block(uint8_t crc, const uint8_t *buffer, size_t len)
{
  do
  {
    crc = _calc_crc(crc, *buffer++);
  } while (--len > 0);
  return crc;
}

void xs_onewire_crc(xsMachine *the)
{
  uint8_t crc = 0;
  uint8_t *src = xsmcToArrayBuffer(xsArg(0));
  uint8_t len = xsGetArrayBufferLength(xsArg(0));
  int argc = xsmcArgc;
  ;
  if (argc > 1)
  {
    uint8_t arg_len = xsmcToInteger(xsArg(1));
    if (arg_len < len)
      len = arg_len;
  }

  crc = _calc_crc_block(crc, src, len);
  xsmcSetInteger(xsResult, crc);
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
28CC2E230500006B:24.1875 read 9: 800115097FFF10107D
28CC2E230500006B:24.375 read 9: 7A0115097FFF0610B0

*/
    int count = xsmcToInteger(xsArg(0));
    xsResult = xsArrayBuffer(NULL, count);
    uint8_t *buffer = xsmcToArrayBuffer(xsResult);

    // hack as 24.1875c ->  800115097FFF10107D
    buffer[0] = 0x75 + rand() % 10;
    buffer[1] = 0x01;
    buffer[2] = 0x15;
    buffer[3] = 0x09;
    buffer[4] = 0x7F;
    buffer[5] = 0xFF;
    buffer[6] = 0x10;
    buffer[7] = 0x10;
    buffer[8] = _calc_crc_block(0, buffer, 8);
  }
}
