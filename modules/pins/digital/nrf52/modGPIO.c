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
#define kResetReasonGPIO (1L << 16)

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
	nrf_gpio_pin_sense_t sense_config = NRF_GPIO_PIN_NOSENSE;
	 
	if (mode & kModGPIOWakeRisingEdge)
		sense_config = NRF_GPIO_PIN_SENSE_HIGH;
	else if (mode & kModGPIOWakeFallingEdge)
		sense_config = NRF_GPIO_PIN_SENSE_LOW;
	mode &= ~(kModGPIOWakeRisingEdge | kModGPIOWakeFallingEdge);
	
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
		case kModGPIOOutputOpenDrain:
			config->direction = 1;
			nrf_gpio_cfg(config->pin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
			break;
		default:
			return -1;
	}

	if (NRF_GPIO_PIN_NOSENSE != sense_config)
		nrf_gpio_cfg_sense_set(config->pin, sense_config);

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

uint8_t modGPIODidWake(modGPIOConfiguration config, uint8_t pin)
{
	if (kResetReasonGPIO == nrf52_get_reset_reason()) {
		if (nrf_gpio_pin_latch_get(pin)) {
			nrf_gpio_pin_latch_clear(pin);
			return 1;
		}
	}
	return 0;
}

