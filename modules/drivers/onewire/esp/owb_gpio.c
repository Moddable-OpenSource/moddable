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

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "xsmc.h"

#if ESP32 || __ets__
	#include "xsHost.h"

#else
	#error unknown platform    
#endif

#include "modGPIO.h"
#include "owb.h"
#include "owb_gpio.h"

// Use for now... sort later... Need to allocate
static modGPIOConfigurationRecord config;

/// @cond ignore
struct _OneWireBus_Timing
{
    uint32_t A, B, C, D, E, F, G, H, I, J;
};
//// @endcond

// 1-Wire timing delays (standard) in microseconds.
// Labels and values are from https://www.maximintegrated.com/en/app-notes/index.mvp/id/126
static const struct _OneWireBus_Timing _StandardTiming = {
        6,    // A - read/write "1" master pull DQ low duration
        64,   // B - write "0" master pull DQ low duration
        60,   // C - write "1" master pull DQ high duration
        10,   // D - write "0" master pull DQ high duration
        9,    // E - read master pull DQ high duration
        55,   // F - complete read timeslot + 10ms recovery
        0,    // G - wait before reset
        480,  // H - master pull DQ low duration
        70,   // I - master pull DQ high duration
        410,  // J - complete presence timeslot + recovery
};

#define info_from_bus(owb) container_of(owb, owb_gpio_driver_info, bus)

/**
 * @brief Generate a 1-Wire reset (initialization).
 * @param[in] bus Initialised bus instance.
 * @param[out] is_present true if device is present, otherwise false.
 * @return status
 */
static owb_status _reset(const OneWireBus * bus, bool * is_present)
{
    bool present = false;
    
    modCriticalSectionBegin();

    owb_gpio_driver_info *i = info_from_bus(bus);

    modGPIOSetMode(bus->config, kModGPIOOutput);

    modGPIOWrite(bus->config, 1); // NEW
    ets_delay_us(bus->timing->G);
    modGPIOWrite(bus->config, 0);  // Drive DQ low
    ets_delay_us(bus->timing->H);
    modGPIOSetMode(bus->config, kModGPIOInput); // Release the bus
    modGPIOWrite(bus->config, 1);  // Reset the output level for the next output
    ets_delay_us(bus->timing->I);

    int level1 = modGPIORead(bus->config);

    ets_delay_us(bus->timing->J);   // Complete the reset sequence recovery

    int level2 = modGPIORead(bus->config);

    modCriticalSectionEnd();

    present = (level1 == 0) && (level2 == 1);   // Sample for presence pulse from slave

    //*is_present = present;

    return OWB_STATUS_OK;
}

/**
 * @brief Send a 1-Wire write bit, with recovery time.
 * @param[in] bus Initialised bus instance.
 * @param[in] bit The value to send.
 */
static void _write_bit(const OneWireBus * bus, int bit)
{
    int delay1 = bit ? bus->timing->A : bus->timing->C;
    int delay2 = bit ? bus->timing->B : bus->timing->D;
    owb_gpio_driver_info *i = info_from_bus(bus);

    modGPIOSetMode(bus->config, kModGPIOOutput);
    
    modGPIOWrite(bus->config, 0);  // Drive DQ low
    modCriticalSectionBegin();
    ets_delay_us(delay1);
    modGPIOWrite(bus->config, 1);  // Release the bus
    ets_delay_us(delay2);

    modCriticalSectionEnd();
}

/**
 * @brief Read a bit from the 1-Wire bus and return the value, with recovery time.
 * @param[in] bus Initialised bus instance.
 */
static int _read_bit(const OneWireBus * bus)
{
    int result = 0;
    owb_gpio_driver_info *i = info_from_bus(bus);

    modGPIOSetMode(bus->config, kModGPIOOutput);
    modGPIOWrite(bus->config, 0);  // Drive DQ low

    modCriticalSectionBegin();
    ets_delay_us(bus->timing->A);
    modGPIOSetMode(bus->config, kModGPIOInput); // Release the bus
    modGPIOWrite(bus->config, 1);  // Reset the output level for the next output
    ets_delay_us(bus->timing->E);

    int level = modGPIORead(bus->config);

    ets_delay_us(bus->timing->F);   // Complete the timeslot and 10us recovery

    modCriticalSectionEnd();

    result = level & 0x01;

    return result;
}

/**
 * @brief Write 1-Wire data byte.
 * NOTE: The data is shifted out of the low bits, eg. it is written in the order of lsb to msb
 * @param[in] bus Initialised bus instance.
 * @param[in] data Value to write.
 * @param[in] number_of_bits_to_read bits to write
 */
static owb_status _write_bits(const OneWireBus * bus, uint8_t data, int number_of_bits_to_write)
{
    for (int i = 0; i < number_of_bits_to_write; ++i)
    {
        _write_bit(bus, data & 0x01);
        data >>= 1;
    }

    return OWB_STATUS_OK;
}

/**
 * @brief Read 1-Wire data byte from  bus.
 * NOTE: Data is read into the high bits, eg. each bit read is shifted down before the next bit is read
 * @param[in] bus Initialised bus instance.
 * @return Byte value read from bus.
 */
static owb_status _read_bits(const OneWireBus * bus, uint8_t *out, int number_of_bits_to_read)
{
    uint8_t result = 0;
    for (int i = 0; i < number_of_bits_to_read; ++i)
    {
        result >>= 1;
        if (_read_bit(bus))
        {
            result |= 0x80;
        }
    }
    *out = result;

    return OWB_STATUS_OK;
}

static owb_status _uninitialize(const OneWireBus * bus)
{
    modGPIOUninit(bus->config);
    c_free(bus->config);
    return OWB_STATUS_OK;
}

static const struct owb_driver gpio_function_table =
{
    .name = "owb_gpio",
    .uninitialize = _uninitialize,
    .reset = _reset,
    .write_bits = _write_bits,
    .read_bits = _read_bits
};

OneWireBus* owb_gpio_initialize(owb_gpio_driver_info *driver_info, int pin)
{
    driver_info->bus.driver = &gpio_function_table;
    driver_info->bus.timing = &_StandardTiming;

    driver_info->bus.config = c_malloc(sizeof(modGPIOConfigurationRecord));
    if ( driver_info->bus.config == NULL ) {
        return NULL;
    }
    if (modGPIOInit(driver_info->bus.config , NULL, pin, kModGPIOOutput)) {
        return NULL;
    }

    return &(driver_info->bus);
}
