/*
 * Copyright (c) 2017-2019  Moddable Tech, Inc.
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

#include "xs.h"
#include "xsHost.h"

#include "modGPIO.h"

/*
	gpio
*/

#define kUninitializedPin (255)

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode)
{
	int result;

	config->pin = pin;

	result = modGPIOSetMode(config, mode);
	if (result) {
		config->pin = kUninitializedPin;
		config->direction = -1;
		return result;
	}

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
	if (0 == config->direction) {
		nrf_gpio_input_disconnect(config->pin);
		config->direction = -1;
	}
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode)
{
	switch (mode) {
		case kModGPIOInput:
			config->direction = 0;
			nrf_gpio_cfg_input(config->pin, NRF_GPIO_PIN_NOPULL);
			break;
		case kModGPIOInputPullUp:
			config->direction = 0;
			nrf_gpio_cfg_input(config->pin, NRF_GPIO_PIN_PULLUP);
			break;
		case kModGPIOInputPullDown:
			config->direction = 0;
			nrf_gpio_cfg_input(config->pin, NRF_GPIO_PIN_PULLDOWN);
			break;
		case kModGPIOOutput:
			config->direction = 1;
			nrf_gpio_cfg_output(config->pin);
			break;
		default:
			return -1;
	}

	return 0;
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
	if (0 == config->direction)
		return nrf_gpio_pin_read(config->pin);
	else if (1 == config->direction)
		return nrf_gpio_pin_out_read(config->pin);
	else
		return 0;
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	nrf_gpio_pin_write(config->pin, value);
}
