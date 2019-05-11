/*
 * MIT License
 *
 * Copyright (c) 2017 David Antliff
 * Copyright (c) 2017 Chris Morgan <chmorgan@gmail.com>
 * Copyright (c) 2019/05/11  Wilberforce for use in Moddable SDK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 */

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "modGPIO.h"

#include "owb.h"
#include "owb_gpio.h"

static bool _is_init(const OneWireBus * bus)
{
    bool ok = false;
    if (bus != NULL)
    {
        if (bus->driver)
        {
            // OK
            ok = true;
        }
        else
        {
            // C "bus is not initialised");
        }
    }
    else
    {
        // "bus is NULL");
    }
    return ok;
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

/**
 * @brief 1-Wire 8-bit CRC lookup.
 * @param[in] crc Starting CRC value. Pass in prior CRC to accumulate.
 * @param[in] data Byte to feed into CRC.
 * @return Resultant CRC value.
 */
static uint8_t _calc_crc(uint8_t crc, uint8_t data)
{
    return crc = (dscrc2x16_table[crc & 0x0f]) ^ (dscrc2x16_table[16 + ((crc >> 4) & 0x0f)]);
}

/**
 * @param[out] is_found true if a device was found, false if not
 * @return status
 */
static owb_status _search(const OneWireBus * bus, OneWireBus_SearchState * state, bool *is_found)
{
    // Based on https://www.maximintegrated.com/en/app-notes/index.mvp/id/187

    // initialize for search
    int id_bit_number = 1;
    int last_zero = 0;
    int rom_byte_number = 0;
    uint8_t id_bit = 0;
    uint8_t cmp_id_bit = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t search_direction = 0;
    bool search_result = false;
    uint8_t crc8 = 0;
    owb_status status;

    // if the last call was not the last one
    if (!state->last_device_flag)
    {
        // 1-Wire reset
        bool is_present;
        bus->driver->reset(bus, &is_present);
        if (!is_present)
        {
            // reset the search
            state->last_discrepancy = 0;
            state->last_device_flag = false;
            state->last_family_discrepancy = 0;
            *is_found = false;
            return OWB_STATUS_OK;
        }

        // issue the search command
        bus->driver->write_bits(bus, OWB_ROM_SEARCH, 8);

        // loop to do the search
        do
        {
            id_bit = cmp_id_bit = 0;

            // read a bit and its complement
            bus->driver->read_bits(bus, &id_bit, 1);
            bus->driver->read_bits(bus, &cmp_id_bit, 1);

            // check for no devices on 1-wire (signal level is high in both bit reads)
            if (id_bit && cmp_id_bit)
            {
                break;
            } else
            {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit)
                {
                    search_direction = (id_bit) ? 1 : 0;  // bit write value for search
                } else
                {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < state->last_discrepancy)
                        search_direction = ((state->rom_code.bytes[rom_byte_number] & rom_byte_mask) > 0);
                    else
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == state->last_discrepancy);

                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0)
                    {
                        last_zero = id_bit_number;

                        // check for Last discrepancy in family
                        if (last_zero < 9)
                            state->last_family_discrepancy = last_zero;
                    }
                }

                // set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_byte_mask
                if (search_direction == 1)
                    state->rom_code.bytes[rom_byte_number] |= rom_byte_mask;
                else
                    state->rom_code.bytes[rom_byte_number] &= ~rom_byte_mask;

                // serial number search direction write bit
                bus->driver->write_bits(bus, search_direction, 1);

                // increment the byte counter id_bit_number
                // and shift the mask rom_byte_mask
                id_bit_number++;
                rom_byte_mask <<= 1;

                // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_byte_mask == 0)
                {
                    crc8 = owb_crc8_byte(crc8, state->rom_code.bytes[rom_byte_number]);  // accumulate the CRC
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        }
        while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

        // if the search was successful then
        if (!((id_bit_number < 65) || (crc8 != 0)))
        {
            // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
            state->last_discrepancy = last_zero;

            // check for last device
            if (state->last_discrepancy == 0)
                state->last_device_flag = true;

            search_result = true;
        }
    }

    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !state->rom_code.bytes[0])
    {
        state->last_discrepancy = 0;
        state->last_device_flag = false;
        state->last_family_discrepancy = 0;
        search_result = false;
    }

    status = OWB_STATUS_OK;

    *is_found = search_result;

    return status;
}

// Public API

owb_status owb_uninitialize(OneWireBus * bus)
{
    owb_status status;

    if(!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bus->driver->uninitialize(bus);
        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_use_crc(OneWireBus * bus, bool use_crc)
{
    owb_status status;

    if(!bus)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bus->use_crc = use_crc;

        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_read_rom(const OneWireBus * bus, OneWireBus_ROMCode *rom_code)
{
    owb_status status;

    memset(rom_code, 0, sizeof(OneWireBus_ROMCode));

    if(!bus || !rom_code)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bool is_present;
        bus->driver->reset(bus, &is_present);
        if (is_present)
        {
            uint8_t value = OWB_ROM_READ;
            bus->driver->write_bits(bus, value, 8);
            owb_read_bytes(bus, rom_code->bytes, sizeof(OneWireBus_ROMCode));

            if (bus->use_crc)
            {
                if (owb_crc8_bytes(0, rom_code->bytes, sizeof(OneWireBus_ROMCode)) != 0)
                {
                    memset(rom_code->bytes, 0, sizeof(OneWireBus_ROMCode));
                    status = OWB_STATUS_CRC_FAILED;
                } else
                {
                    status = OWB_STATUS_OK;
                }
            } else
            {
                status = OWB_STATUS_OK;
            }
        }
        else
        {
            status = OWB_STATUS_DEVICE_NOT_RESPONDING;
        }
    }

    return status;
}

owb_status owb_verify_rom(const OneWireBus * bus, OneWireBus_ROMCode rom_code, bool* is_present)
{
    owb_status status;
    bool result = false;

    if (!bus || !is_present)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    }
    else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    }
    else
    {
        OneWireBus_SearchState state = {
            .last_discrepancy = 64,
            .last_device_flag = false,
        };

        bool is_found = false;
        while (!state.last_device_flag && !is_found)
        {
            _search(bus, &state, &is_found);
            if (is_found)
            {
                result = true;
                for (int i = 0; i < sizeof(state.rom_code.bytes) && result; ++i)
                {
                    result = rom_code.bytes[i] == state.rom_code.bytes[i];
                    // ("%02x %02x", rom_code.bytes[i], state.rom_code.bytes[i]);
                }
                is_found = result;
            }
        }

        // ("rom code %sfound", result ? "" : "not ");
        *is_present = result;
        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_reset(const OneWireBus * bus, bool* a_device_present)
{
    owb_status status;

    if(!bus || !a_device_present)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if(!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bus->driver->reset(bus, a_device_present);
        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_write_byte(const OneWireBus * bus, uint8_t data)
{
    owb_status status;

    if(!bus)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bus->driver->write_bits(bus, data, 8);
        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_read_byte(const OneWireBus * bus, uint8_t *out)
{
    owb_status status;

    if(!bus || !out)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        bus->driver->read_bits(bus, out, 8);
        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_read_bytes(const OneWireBus * bus, uint8_t * buffer, unsigned int len)
{
    owb_status status;

    if(!bus || !buffer)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        for (int i = 0; i < len; ++i)
        {
            uint8_t out;
            bus->driver->read_bits(bus, &out, 8);
            buffer[i] = out;
        }

        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_write_bytes(const OneWireBus * bus, const uint8_t * buffer, unsigned int len)
{
    owb_status status;

    if(!bus || !buffer)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        for (int i = 0; i < len; i++)
        {
            bus->driver->write_bits(bus, buffer[i], 8);
        }

        status = OWB_STATUS_OK;
    }

    return status;
}

owb_status owb_write_rom_code(const OneWireBus * bus, OneWireBus_ROMCode rom_code)
{
    owb_status status;

    if(!bus)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        owb_write_bytes(bus, (uint8_t*)&rom_code, sizeof(rom_code));
        status = OWB_STATUS_OK;
    }

    return status;
}

uint8_t owb_crc8_byte(uint8_t crc, uint8_t data)
{
    return _calc_crc(crc, data);
}

uint8_t owb_crc8_bytes(uint8_t crc, const uint8_t * data, size_t len)
{
    return _calc_crc_block(crc, data, len);
}

owb_status owb_search_first(const OneWireBus * bus, OneWireBus_SearchState * state, bool* found_device)
{
    bool result;
    owb_status status;

    if(!bus || !state || !found_device)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        memset(&state->rom_code, 0, sizeof(state->rom_code));
        state->last_discrepancy = 0;
        state->last_family_discrepancy = 0;
        state->last_device_flag = false;
        _search(bus, state, &result);
        status = OWB_STATUS_OK;

        *found_device = result;
    }

    return status;
}

owb_status owb_search_next(const OneWireBus * bus, OneWireBus_SearchState * state, bool* found_device)
{
    owb_status status;
    bool result = false;

    if(!bus || !state || !found_device)
    {
        status = OWB_STATUS_PARAMETER_NULL;
    } else if (!_is_init(bus))
    {
        status = OWB_STATUS_NOT_INITIALIZED;
    } else
    {
        _search(bus, state, &result);
        status = OWB_STATUS_OK;

        *found_device = result;
    }

    return status;
}