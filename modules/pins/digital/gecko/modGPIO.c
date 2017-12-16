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

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode)
{
	int result;
	uint32_t portNum = (uint32_t)port;

	if (portNum > 7) {				// sent numeric
		portNum = port[8] - 'A';	// sent string "gpioPortX"
	}

	if (portNum > 7) {
		config->pin = kUninitializedPin;
		return -1;
	}

	config->portNum = portNum;
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
	switch (mode) {
		case kModGPIOInput:
			GPIO_PinModeSet(config->portNum, config->pin, gpioModeInput, 0);
			break;
		case kModGPIOInputPullUp:
		case kModGPIOInputPullDown:
			GPIO_PinModeSet(config->portNum, config->pin, gpioModeInputPull, mode == kModGPIOInputPullUp ? 1 : 0);
			break;
		case kModGPIOOutput:
			GPIO_PinModeSet(config->portNum, config->pin, gpioModePushPull, 1);
			break;
		case kModGPIOOutputOpenDrain:
			GPIO_PinModeSet(config->portNum, config->pin, gpioModeWiredAnd, 0);
			break;
		default:
			return -1;
	}
	return 0;
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
