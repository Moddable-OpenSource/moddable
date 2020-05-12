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

#pragma once
#ifndef OWB_RMT_H
#define OWB_RMT_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "driver/rmt.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  int tx_channel;     ///< RMT channel to use for TX
  int rx_channel;     ///< RMT channel to use for RX
  RingbufHandle_t rb; ///< Ring buffer handle
  int gpio;           ///< OneWireBus GPIO
  OneWireBus bus;     ///< OneWireBus instance
} owb_rmt_driver_info;

/**
 * @brief Initialise the RMT driver.
 * @return OneWireBus*, pass this into the other OneWireBus public API functions
 */
OneWireBus* owb_rmt_initialize(owb_rmt_driver_info *info, uint8_t gpio_num,
                               rmt_channel_t tx_channel, rmt_channel_t rx_channel);

#ifdef __cplusplus
}
#endif

#endif // OWB_RMT_H
