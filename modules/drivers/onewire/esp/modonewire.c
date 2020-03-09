/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
 * Copyright (c) 2019  Wilberforce
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

#include "modGPIO.h"

#include "owb.h"
#include "owb_gpio.h"

#ifdef ESP32

#include "owb_rmt.h"

#ifndef MODDEF_ONEWIRE_RMT_TX_CHANNEL
#define MODDEF_ONEWIRE_RMT_TX_CHANNEL (RMT_CHANNEL_3)
#endif

#ifndef MODDEF_ONEWIRE_RMT_RX_CHANNEL
#define MODDEF_ONEWIRE_RMT_RX_CHANNEL (RMT_CHANNEL_2)
#endif

#define MODDEF_ONEWIRE_DRIVER_RMT RMT

#else
#define MODDEF_ONEWIRE_DRIVER_GPIO GPIO
#endif

typedef struct
{
  xsSlot obj;
  uint8_t pin;

  OneWireBus *owb;

#ifdef MODDEF_ONEWIRE_DRIVER_GPIO 
  owb_gpio_driver_info driver_info;
#endif
#ifdef MODDEF_ONEWIRE_DRIVER_RMT
  owb_rmt_driver_info driver_info;
#endif

} modOneWireRecord, *modOneWire;

void xs_onewire_destructor(void *data)
{
  modOneWire onewire = data;
  if (NULL == onewire)
    return;
  owb_uninitialize(onewire->owb);
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

  xsRemember(onewire->obj);

#ifdef MODDEF_ONEWIRE_DRIVER_GPIO
  onewire->owb = owb_gpio_initialize(&onewire->driver_info, onewire->pin);
#endif
#ifdef MODDEF_ONEWIRE_DRIVER_RMT
  onewire->owb = owb_rmt_initialize(&onewire->driver_info, onewire->pin, MODDEF_ONEWIRE_RMT_TX_CHANNEL, MODDEF_ONEWIRE_RMT_RX_CHANNEL);
#endif
  if (onewire->owb == NULL)
  {
    xsUnknownError("can't init pin");
  }
  owb_use_crc(onewire->owb, true); // enable CRC check for ROM code

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

  int argc = xsmcArgc;

  if (argc == 0) // Read a byte
  {
    uint8_t value = 0;
    owb_read_byte(onewire->owb, &value);
    xsmcSetInteger(xsResult, value);
  }
  else
  {
    int count = xsmcToInteger(xsArg(0));
    xsmcSetArrayBuffer(xsResult, NULL, count);
    uint8_t *buffer = xsmcToArrayBuffer(xsResult);
    owb_read_bytes(onewire->owb, (uint8_t *)buffer, count);
  }
}

void xs_onewire_write(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  uint8_t value = xsmcToInteger(xsArg(0));
  if ((value < 0) || (value > 255))
    xsRangeError("bad value");
  owb_write_byte(onewire->owb, value);
}

void xs_onewire_select(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  OneWireBus_ROMCode *rom_code = xsmcToArrayBuffer(xsArg(0));
  bool present = false;
  owb_reset(onewire->owb, &present);
  owb_write_byte(onewire->owb, OWB_ROM_MATCH);
  owb_write_rom_code(onewire->owb, *rom_code);
}

void xs_onewire_search(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);

  OneWireBus_SearchState search_state = {0};
  bool found = false;
  owb_search_first(onewire->owb, &search_state, &found);
  xsmcVars(1);
  xsResult = xsNewArray(0);

  while (found)
  {
	xsmcSetArrayBuffer(xsVar(0), &search_state.rom_code.bytes, 8);
    xsCall1(xsResult, xsID_push, xsVar(0));
    owb_search_next(onewire->owb, &search_state, &found);
  }
}

void xs_onewire_isPresent(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);

  OneWireBus_SearchState search_state = {0};
  bool found = false;
  uint8_t *id;

  if (8 != xsmcGetArrayBufferLength(xsArg(0)))
    xsUnknownError("invalid id");

  id = xsmcToArrayBuffer(xsArg(0));

  owb_search_first(onewire->owb, &search_state, &found);
  while (found)
  {
    if (0 == memcmp(search_state.rom_code.bytes, id, 8))
    {
      xsResult = xsTrue;
      return;
    }
    owb_search_next(onewire->owb, &search_state, &found);
  }

  xsResult = xsFalse;
}

void xs_onewire_reset(xsMachine *the)
{
  modOneWire onewire = xsmcGetHostData(xsThis);
  bool present = false;
  owb_reset(onewire->owb, &present);
  xsmcSetBoolean(xsResult, present);
}

void xs_onewire_crc(xsMachine *the)
{
  uint8_t crc = 0;
  uint8_t *src = xsmcToArrayBuffer(xsArg(0));
  uint8_t len = xsmcGetArrayBufferLength(xsArg(0));
  int argc = xsmcArgc;

  if (argc > 1)
  {
    size_t arg_len = xsmcToInteger(xsArg(1));
    if (arg_len < len)
      len = arg_len;
  }

  crc = owb_crc8_bytes(crc, src, len);
  xsmcSetInteger(xsResult, crc);
}
