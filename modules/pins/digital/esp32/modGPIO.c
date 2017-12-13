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

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode)
{
	int result;

	if ((pin > GPIO_NUM_MAX) || port) {
		config->pin = kUninitializedPin;
		return -1;
	}

	config->pin = pin;
	result = modGPIOSetMode(config, mode);
	if (result) {
		config->pin = kUninitializedPin;
		return -1;
	}

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode)
{
	switch (mode) {
		case kModGPIOOutput:
			if (config->pin >= GPIO_NUM_34)		// pins 34-39 are input only
				return -1;

			GPIO_INIT_OUTPUT(config->pin);
			GPIO_CLEAR(config->pin);
			break;

		case kModGPIOInput:
		case kModGPIOInputPullUp:
		case kModGPIOInputPullDown:
		case kModGPIOInputPullUpDown:
			GPIO_INIT_INPUT(config->pin);

			if (kModGPIOInputPullUp == mode)
				gpio_set_pull_mode(config->pin, GPIO_PULLUP_ONLY);
			else if (kModGPIOInputPullDown == mode)
				gpio_set_pull_mode(config->pin, GPIO_PULLDOWN_ONLY);
			else if (kModGPIOInputPullUpDown == mode)
				gpio_set_pull_mode(config->pin, GPIO_PULLUP_PULLDOWN);
			else
				gpio_set_pull_mode(config->pin, GPIO_FLOATING);
			break;

		default:
			return -1;
	}

	return 0;
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
	return gpio_get_level(config->pin);
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	gpio_set_level(config->pin, value ? 1 : 0);
}
