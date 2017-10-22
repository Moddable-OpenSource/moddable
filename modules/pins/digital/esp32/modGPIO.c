/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsesp.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "modGPIO.h"

/*
	gpio
*/

#define GPIO_INIT_OUTPUT(index) \
		gpio_pad_select_gpio(index); \
		gpio_set_direction(index, GPIO_MODE_OUTPUT)

#define GPIO_INIT_INPUT(index) \
		gpio_pad_select_gpio(index); \
		gpio_set_direction(index, GPIO_MODE_INPUT)

#define GPIO_CLEAR(index) (gpio_set_level(index, 0))
#define GPIO_SET(index) (gpio_set_level(index, 1))

#define kUninitializedPin (255)

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint8_t mode)
{
	config->pin = kUninitializedPin;

	if ((pin > GPIO_NUM_MAX) || port)
		return -1;

	if (kModGPIOOutput == mode) {
		if (pin < GPIO_NUM_34) {		// pins 34-39 are input only
			GPIO_INIT_OUTPUT(pin);
			GPIO_CLEAR(pin);
		}
		else 
			return -1;
	}
	else if (kModGPIOInput == mode) {
		GPIO_INIT_INPUT(pin);
	}
	else
		return -1;

	config->pin = pin;

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint8_t mode)
{
	return modGPIOInit(config, NULL, config->pin, mode);
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
	if (config->pin < GPIO_NUM_MAX)
		return (gpio_get_level(config->pin));

	return 0xff;
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	if (config->pin < GPIO_NUM_MAX) {
		gpio_set_level(config->pin, value ? 1 : 0);
	}
}
