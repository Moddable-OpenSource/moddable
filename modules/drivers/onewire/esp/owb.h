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
 * @brief Interface definitions for the 1-Wire bus component.
 *
 * This component provides structures and functions that are useful for communicating
 * with devices connected to a Maxim Integrated 1-Wire® bus via a single GPIO.
 *
 * Currently only externally powered devices are supported. Parasitic power is not supported.
 */

#ifndef ONE_WIRE_BUS_H
#define ONE_WIRE_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "modGPIO.h"

#ifdef __cplusplus
extern "C" {
#endif


// ROM commands
#define OWB_ROM_SEARCH        0xF0
#define OWB_ROM_READ          0x33
#define OWB_ROM_MATCH         0x55
#define OWB_ROM_SKIP          0xCC
#define OWB_ROM_SEARCH_ALARM  0xEC

#define OWB_ROM_CODE_STRING_LENGTH (17)  ///< Typical length of OneWire bus ROM ID as ASCII hex string, including null terminator

struct owb_driver;

/**
 * @brief Structure containing 1-Wire bus information relevant to a single instance.
 */
typedef struct
{
    const struct _OneWireBus_Timing * timing;   ///< Pointer to timing information
    bool use_crc;                               ///< True if CRC checks are to be used when retrieving information from a device on the bus

    const struct owb_driver *driver;
    modGPIOConfiguration config; ///< Value of the GPIO connected to the 1-Wire bus
} OneWireBus;

/**
 * @brief Represents a 1-Wire ROM Code. This is a sequence of eight bytes, where
 *        the first byte is the family number, then the following 6 bytes form the
 *        serial number. The final byte is the CRC8 check byte.
 */
typedef union
{
    /// Provides access via field names
    struct fields
    {
        uint8_t family[1];         ///< family identifier (1 byte, LSB - read/write first)
        uint8_t serial_number[6];  ///< serial number (6 bytes)
        uint8_t crc[1];            ///< CRC check byte (1 byte, MSB - read/write last)
    } fields;                      ///< Provides access via field names

    uint8_t bytes[8];              ///< Provides raw byte access

} OneWireBus_ROMCode;

/**
 * @brief Represents the state of a device search on the 1-Wire bus.
 *
 *        Pass a pointer to this structure to owb_search_first() and
 *        owb_search_next() to iterate through detected devices on the bus.
 */
typedef struct
{
    OneWireBus_ROMCode rom_code;
    int last_discrepancy;
    int last_family_discrepancy;
    int last_device_flag;
} OneWireBus_SearchState;

typedef enum
{
    OWB_STATUS_OK,
    OWB_STATUS_NOT_INITIALIZED,
    OWB_STATUS_PARAMETER_NULL,
    OWB_STATUS_DEVICE_NOT_RESPONDING,
    OWB_STATUS_CRC_FAILED,
    OWB_STATUS_TOO_MANY_BITS,
    OWB_STATUS_HW_ERROR
} owb_status;

/** NOTE: Driver assumes that (*init) was called prior to any other methods */
struct owb_driver
{
    const char* name;

    owb_status (*uninitialize)(const OneWireBus * bus);

    owb_status (*reset)(const OneWireBus * bus, bool *is_present);

    /** NOTE: The data is shifted out of the low bits, eg. it is written in the order of lsb to msb */
    owb_status (*write_bits)(const OneWireBus *bus, uint8_t out, int number_of_bits_to_write);

    /** NOTE: Data is read into the high bits, eg. each bit read is shifted down before the next bit is read */
    owb_status (*read_bits)(const OneWireBus *bus, uint8_t *in, int number_of_bits_to_read);
};

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * @brief call to release resources after completing use of the OneWireBus
 */
owb_status owb_uninitialize(OneWireBus * bus);

/**
 * @brief Enable or disable use of CRC checks on device communications.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in] use_crc True to enable CRC checks, false to disable.
 * @return status
 */
owb_status owb_use_crc(OneWireBus * bus, bool use_crc);

/**
 * @brief Read ROM code from device - only works when there is a single device on the bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[out] rom_code the value read from the device's rom
 * @return status
 */
owb_status owb_read_rom(const OneWireBus * bus, OneWireBus_ROMCode *rom_code);

/**
 * @brief Verify the device specified by ROM code is present.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in] rom_code ROM code to verify.
 * @param[out] is_present set to true if a device is present, false if not
 * @return status
 */
owb_status owb_verify_rom(const OneWireBus * bus, OneWireBus_ROMCode rom_code, bool* is_present);

/**
 * @brief Reset the 1-Wire bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[out] is_present set to true if at least one device is present on the bus
 * @return status
 */
owb_status owb_reset(const OneWireBus * bus, bool* a_device_present);

/**
 * @brief Write a single byte to the 1-Wire bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in] data Byte value to write to bus.
 * @return status
 */
owb_status owb_write_byte(const OneWireBus * bus, uint8_t data);

/**
 * @brief Read a single byte from the 1-Wire bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[out] out The byte value read from the bus.
 * @return status
 */
owb_status owb_read_byte(const OneWireBus * bus, uint8_t *out);

/**
 * @brief Read a number of bytes from the 1-Wire bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in, out] buffer Pointer to buffer to receive read data.
 * @param[in] len Number of bytes to read, must not exceed length of receive buffer.
 * @return status.
 */
owb_status owb_read_bytes(const OneWireBus * bus, uint8_t * buffer, size_t len);

/**
 * @brief Write a number of bytes to the 1-Wire bus.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in] buffer Pointer to buffer to write data from.
 * @param[in] len Number of bytes to write.
 * @return status
 */
owb_status owb_write_bytes(const OneWireBus * bus, const uint8_t * buffer, size_t len);

/**
 * @brief Write a ROM code to the 1-Wire bus ensuring LSB is sent first.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in] rom_code ROM code to write to bus.
 * @return status
 */
owb_status owb_write_rom_code(const OneWireBus * bus, OneWireBus_ROMCode rom_code);

/**
 * @brief 1-Wire 8-bit CRC lookup.
 * @param[in] crc Starting CRC value. Pass in prior CRC to accumulate.
 * @param[in] data Byte to feed into CRC.
 * @return Resultant CRC value.
 *         Should be zero if last byte was the CRC byte and the CRC matches.
 */
uint8_t owb_crc8_byte(uint8_t crc, uint8_t data);

/**
 * @brief 1-Wire 8-bit CRC lookup with accumulation over a block of bytes.
 * @param[in] crc Starting CRC value. Pass in prior CRC to accumulate.
 * @param[in] data Array of bytes to feed into CRC.
 * @param[in] len Length of data array in bytes.
 * @return Resultant CRC value.
 *         Should be zero if last byte was the CRC byte and the CRC matches.
 */
uint8_t owb_crc8_bytes(uint8_t crc, const uint8_t * data, size_t len);

/**
 * @brief Locates the first device on the 1-Wire bus, if present.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in,out] state Pointer to an existing search state structure.
 * @param[out] found_device True if a device is found, false if no devices are found.
 *         If a device is found, the ROM Code can be obtained from the state.
 * @return status
 */
owb_status owb_search_first(const OneWireBus * bus, OneWireBus_SearchState * state, bool *found_device);

/**
 * @brief Locates the next device on the 1-Wire bus, if present, starting from
 *        the provided state. Further calls will yield additional devices, if present.
 * @param[in] bus Pointer to initialised bus instance.
 * @param[in,out] state Pointer to an existing search state structure.
 * @param[out] found_device True if a device is found, false if no devices are found.
 *         If a device is found, the ROM Code can be obtained from the state.
 * @return status
 */
owb_status owb_search_next(const OneWireBus * bus, OneWireBus_SearchState * state, bool *found_device);

#include "owb_gpio.h"

#ifdef __cplusplus
}
#endif

#endif  // ONE_WIRE_BUS_H
