/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

#include "xsHost.h"

#include "hardware/gpio.h"

#include "modGPIO.h"

/*
	gpio
*/
#define kUninitializedPin (255)

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode)
{
	int result;

#if WIFI_GPIO
	if (pin == 32) {
		if ((mode != kModGPIOInput) && (mode != kModGPIOOutput)) {
			config->pin = kUninitializedPin;
			return -1;
		}
		pico_use_cyw43();
	}
	else
#endif
	if ((pin > 29) || port) {
		config->pin = kUninitializedPin;
		return -1;
	}
	else
		gpio_init(pin);

	config->pin = pin;

	result = modGPIOSetMode(config, mode);
	if (result) {
		config->pin = kUninitializedPin;
		return result;
	}

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
#if WIFI_GPIO
	if (config->pin == 32) {
		pico_unuse_cyw43();
	}
	else
#endif
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode)
{
#if WIFI_GPIO
	if (config->pin == 32)
		return 0;
#endif
	gpio_set_function(config->pin, GPIO_FUNC_SIO);

	switch (mode) {
		case kModGPIOInput:
			gpio_set_dir(config->pin, GPIO_IN);
			gpio_set_pulls(config->pin, false, false);
			break;
		case kModGPIOInputPullUp:
			gpio_set_dir(config->pin, GPIO_IN);
			gpio_set_pulls(config->pin, true, false);
			break;
		case kModGPIOInputPullDown:
			gpio_set_dir(config->pin, GPIO_IN);
			gpio_set_pulls(config->pin, false, true);
			break;

		case kModGPIOOutput:
		case kModGPIOOutputOpenDrain:
			gpio_set_dir(config->pin, GPIO_OUT);
			break;

		default:
			return -1;
	}

	return 0;
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
#if WIFI_GPIO
	if (config->pin == 32) {
		if (cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN))
			return 1;
		else
			return 0;
	}
#endif
	return gpio_get(config->pin);
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
#if WIFI_GPIO
	if (config->pin == 32) {
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value);
	}
#endif
	gpio_put(config->pin, value);
}
