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
#include "xsHost.h"

#include "modGPIO.h"
#include "mc.defines.h"
#include "owb.h"
#include "owb_gpio.h"

#if __ets__ && !ESP32 && !defined(MODDEF_ONEWIRE_DIRECTGPIO)
	// esp8266 defaults to MODDEF_ONEWIRE_DIRECTGPIO 
	#define MODDEF_ONEWIRE_DIRECTGPIO 1
#endif

#if MODDEF_ONEWIRE_DIRECTGPIO
	#define PIN_TO_BASEREG(pin)             ((volatile uint32_t*) GPO)
	#define PIN_TO_BITMASK(pin)             (1 << pin)
	#define IO_REG_TYPE uint32_t
	#define IO_REG_BASE_ATTR
	#define IO_REG_MASK_ATTR
	#define DIRECT_READ(base, mask)         ((GPI & (mask)) ? 1 : 0)    //GPIO_IN_ADDRESS
	#define DIRECT_MODE_INPUT(base, mask)   (GPE &= ~(mask))            //GPIO_ENABLE_W1TC_ADDRESS
	#define DIRECT_MODE_OUTPUT(base, mask)  (GPE |= (mask))             //GPIO_ENABLE_W1TS_ADDRESS
	#define DIRECT_WRITE_LOW(base, mask)    (GPOC = (mask))             //GPIO_OUT_W1TC_ADDRESS
	#define DIRECT_WRITE_HIGH(base, mask)   (GPOS = (mask))             //GPIO_OUT_W1TS_ADDRESS
#endif

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
// Mirror: http://www.pyfn.com/datasheets/maxim/AN126.pdf
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
    *is_present = false;

    struct modGPIOConfigurationRecord *config = bus->config;
#if MODDEF_ONEWIRE_DIRECTGPIO
	volatile IO_REG_TYPE *reg IO_REG_BASE_ATTR = PIN_TO_BASEREG(config->pin);
#endif
	uint8_t retries = 125;

    modCriticalSectionBegin();
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_INPUT(reg, PIN_TO_BITMASK(config->pin));
#else
	modGPIOSetMode(config, kModGPIOInput);
#endif
    modCriticalSectionEnd();
	// wait until the wire is high... just in case
	while (!modGPIORead(config)) {
		ets_delay_us(2);
		if (--retries == 0) {
			modLog("wire never high");
			return OWB_STATUS_OK;
		}
	}

	modCriticalSectionBegin();
	modGPIOWrite(config, 0);
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_OUTPUT(REG, PIN_TO_BITMASK(config->pin));
#else
	modGPIOSetMode(config, kModGPIOOutputOpenDrain);	// drive output low
#endif
	modCriticalSectionEnd();
	ets_delay_us(bus->timing->H);
	modCriticalSectionBegin();
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_INPUT(reg, PIN_TO_BITMASK(config->pin));	// allow it to float
#else
	modGPIOSetMode(config, kModGPIOInput);
#endif
	ets_delay_us(bus->timing->I);
	if (!modGPIORead(config))
		*is_present = true;
	modCriticalSectionEnd();
	ets_delay_us(bus->timing->J);
    return OWB_STATUS_OK;
}

/**
 * @brief Send a 1-Wire write bit, with recovery time.
 * @param[in] bus Initialised bus instance.
 * @param[in] bit The value to send.
 */
static void _write_bit(const OneWireBus * bus, int bit)
{
    struct modGPIOConfigurationRecord *config = bus->config;

    modCriticalSectionBegin();

	modGPIOWrite(config, 0);  // Drive DQ low
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_OUTPUT(REG, PIN_TO_BITMASK(config->pin));
#else
	modGPIOSetMode(config, kModGPIOOutputOpenDrain);
#endif
	if (bit) {
		ets_delay_us(bus->timing->A);
		modGPIOWrite(config, 1);  // Release the bus
		modCriticalSectionEnd();
		ets_delay_us(bus->timing->B);
	}
	else {
		ets_delay_us(bus->timing->C);
		modGPIOWrite(config, 1);  // Release the bus
		modCriticalSectionEnd();
		ets_delay_us(bus->timing->D);
	}
}

/**
 * @brief Read a bit from the 1-Wire bus and return the value, with recovery time.
 * @param[in] bus Initialised bus instance.
 */
static int _read_bit(const OneWireBus * bus)
{
    struct modGPIOConfigurationRecord *config = bus->config;
#if MODDEF_ONEWIRE_DIRECTGPIO
	volatile IO_REG_TYPE *reg IO_REG_BASE_ATTR = PIN_TO_BASEREG(config->pin);
#endif

    modCriticalSectionBegin();
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_OUTPUT(REG, PIN_TO_BITMASK(config->pin));
#else
    modGPIOSetMode(config, kModGPIOOutputOpenDrain);
#endif
    modGPIOWrite(config, 0);  // Drive DQ low

    ets_delay_us(bus->timing->A);
#if MODDEF_ONEWIRE_DIRECTGPIO
	DIRECT_MODE_INPUT(reg, PIN_TO_BITMASK(config->pin)); // Release the bus
#else
    modGPIOSetMode(config, kModGPIOInput);
#endif
    ets_delay_us(bus->timing->E);

    int level = modGPIORead(config);

    modCriticalSectionEnd();

    ets_delay_us(bus->timing->F);   // Complete the timeslot and 10us recovery

    return level & 0x01;
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
    if (modGPIOInit(driver_info->bus.config , NULL, pin, kModGPIOInput)) {
        return NULL;
    }

    return &(driver_info->bus);
}
