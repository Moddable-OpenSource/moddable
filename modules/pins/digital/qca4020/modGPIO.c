/*
 * Copyright (c) 2017-2018  Moddable Tech, Inc.
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
		return result;
	}

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode)
{
	qapi_TLMM_Config_t	tlmm_config;
	qapi_Status_t		status = QAPI_OK;

	tlmm_config.pin = config->pin;
	tlmm_config.func = 0;
	tlmm_config.drive = QAPI_GPIO_2MA_E;
	tlmm_config.pull = QAPI_GPIO_NO_PULL_E;

	switch (mode) {
		case kModGPIOInput:
			tlmm_config.dir = QAPI_GPIO_INPUT_E;
			break;
		case kModGPIOInputPullUp:
			tlmm_config.dir = QAPI_GPIO_INPUT_E;
			tlmm_config.pull = QAPI_GPIO_PULL_UP_E;
			break;
		case kModGPIOInputPullDown:
			tlmm_config.dir = QAPI_GPIO_INPUT_E;
			tlmm_config.pull = QAPI_GPIO_PULL_DOWN_E;
			break;
		case kModGPIOOutput:
			tlmm_config.dir = QAPI_GPIO_OUTPUT_E;
			break;
		case kModGPIOOutputOpenDrain:
			tlmm_config.dir = QAPI_GPIO_OUTPUT_E;
			break;
		default:
			return -1;
	}

//tlmm_config.pull = QAPI_GPIO_NO_PULL_E;
	status = qapi_TLMM_Get_Gpio_ID(&tlmm_config, &config->gpio_id);
	qca4020_error("TLMM_Get_Gpio_ID", status);

	status = qapi_TLMM_Config_Gpio(config->gpio_id, &tlmm_config);
	qca4020_error("TLMM_Config_Gpio", status);
	
	return 0;
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
	qapi_GPIO_Value_t	ret;

	ret = qapi_TLMM_Read_Gpio(config->gpio_id, config->pin);
	return ret == QAPI_GPIO_HIGH_VALUE_E ? 1 : 0;
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	qapi_Status_t status;

	if (value) {
		status = qapi_TLMM_Drive_Gpio(config->gpio_id, config->pin, QAPI_GPIO_HIGH_VALUE_E);
		qca4020_error("TLMM_Drive_Gpio [HIGH]", status);
	}
	else {
		status = qapi_TLMM_Drive_Gpio(config->gpio_id, config->pin, QAPI_GPIO_LOW_VALUE_E);
		qca4020_error("TLMM_Drive_Gpio [LOW]", status);
	}
}
