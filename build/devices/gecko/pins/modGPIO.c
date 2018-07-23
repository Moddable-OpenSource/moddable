/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xs.h"
#include "xsPlatform.h"
#include "xsgecko.h"

#include "modGPIO.h"

#define LEDPORT		gpioPortE
/*
	gpio
*/

#define kUninitializedPin (255)

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint8_t mode)
{
	uint8_t portNum = (uint8_t)port;

	config->pin = kUninitializedPin;
	config->portNum = portNum;

	if (kModGPIOOutput == mode) {
		GPIO_PinModeSet(portNum, pin, gpioModePushPull, 1);
	}
	else if (kModGPIOInput == mode) {
		GPIO_PinModeSet(portNum, pin, gpioModeInput, 0);
	}
/*
	else if (kModGPIOInputPullUp == mode) {
		GPIO_PinModeSet(portNum, pin, gpioModeInputPull, 1);
	}
	else if (kModGPIOInputPullDown == mode) {
		GPIO_PinModeSet(portNum, pin, gpioModeInputPull, 0);
	}
	else if (kModGPIOOutputOpenDrain == mode) {
		GPIO_PinModeSet(portNum, pin, gpioModeWiredAnd, 0);
	}
*/

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
	unsigned int ret;
	ret = GPIO_PinInGet(config->portNum, config->pin);
	return ret;
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	if (value)
		GPIO_PinOutSet(config->portNum, config->pin);
	else
		GPIO_PinOutClear(config->portNum, config->pin);
}
