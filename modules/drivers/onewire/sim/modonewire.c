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
//  modOneWire onewire = xsmcGetHostData(xsThis);
}

void xs_onewire_search(xsMachine *the)
{
//  modOneWire onewire = xsmcGetHostData(xsThis);

  xsmcVars(1);
  xsResult = xsNewArray(0);

  uint8_t buffer[] = {0x28, 0xCA, 0x00, 0xA9, 0x04, 0x00, 0x00, 0xEA};
  xsmcSetArrayBuffer(xsVar(0), buffer, 8);
  xsCall1(xsResult, xsID_push, xsVar(0));

  buffer[6] = 0x78;
  buffer[3] = 0x92;
  xsmcSetArrayBuffer(xsVar(0), buffer, 8);
  xsCall1(xsResult, xsID_push, xsVar(0));
}

void xs_onewire_isPresent(xsMachine *the)
{
//  modOneWire onewire = xsmcGetHostData(xsThis);

//  uint8_t *id;

  if (8 != xsmcGetArrayBufferLength(xsArg(0)))
    xsUnknownError("invalid id");

//  id = xsmcToArrayBuffer(xsArg(0));

  if (0 == rand() % 2)
  {
    xsResult = xsTrue;
    return;
  }
  xsResult = xsFalse;
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
  Dow-CRC using polynomial X^8 + X^5 + X^4 + X^0
 Tiny 2x16 entry CRC table created by Arjen Lentz
 See http://lentz.com.au/blog/calculating-crc-with-a-tiny-32-entry-lookup-table
 */
static const uint8_t dscrc2x16_table[] = {
	0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
	0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
	0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
	0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

static uint8_t _calc_crc_block(uint8_t crc, const uint8_t *buffer, size_t len)
{
	while (len--) {
		crc = *buffer++ ^ crc;  // just re-using crc as intermediate
		crc = (dscrc2x16_table[crc & 0x0f]) ^ (dscrc2x16_table[16 + ((crc >> 4) & 0x0f)]);
	}
  return crc;
}

void xs_onewire_crc(xsMachine *the)
{
  uint8_t crc = 0;
  uint8_t *src = xsmcToArrayBuffer(xsArg(0));
  uint8_t len = xsmcGetArrayBufferLength(xsArg(0));
  int argc = xsmcArgc;
  
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
//  modOneWire onewire = xsmcGetHostData(xsThis);

  int argc = xsmcArgc;
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
    xsmcSetArrayBuffer(xsResult, NULL, count);
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
