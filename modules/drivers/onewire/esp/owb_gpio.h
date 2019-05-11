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
#ifndef OWB_GPIO_H
#define OWB_GPIO_H

#include "owb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    //int gpio;         ///< Value of the GPIO connected to the 1-Wire bus
    //modGPIOConfiguration config; ///< Value of the GPIO connected to the 1-Wire bus
    OneWireBus bus;   ///< OneWireBus instance
} owb_gpio_driver_info;

/**
 * @brief Initialise the GPIO driver.
 * @return OneWireBus*, pass this into the other OneWireBus public API functions
 */
OneWireBus * owb_gpio_initialize(owb_gpio_driver_info *driver_info, int gpio);

/**
 * @brief Clean up after a call to owb_gpio_initialize()
 */
void owb_gpio_uninitialize(owb_gpio_driver_info *driver_info);

#ifdef __cplusplus
}
#endif

#endif // OWB_GPIO_H