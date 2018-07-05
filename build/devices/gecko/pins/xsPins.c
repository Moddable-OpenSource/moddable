/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xs.h"
#include "xsPlatform.h"
#include "xsgecko.h"
#include "mc.xs.h"			// for xsID_ values
#define ID(symbol) (xsID_##symbol)

#define true	1
#define false	0

#include "modGPIO.h"

//#define LEDPORT geckoDefaultGPIOPort

/*
	Digital
*/

void xs_digital_configure(xsMachine *the)
{
}

void xs_digital_read(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	uint8_t value;
	modGPIOConfigurationRecord config;

//	if (modGPIOInit(&config, (const char *)LEDPORT, pin, kModGPIOInput))
		xsUnknownError("can't init pin");

	value = modGPIORead(&config);
	modGPIOUninit(&config);

	if (255 == value)
		xsUnknownError("can't read pin");

	xsResult = xsInteger(value);
}

void xs_digital_write(xsMachine *the)
{
	int pin = xsToInteger(xsArg(0));
	int value = xsToInteger(xsArg(1));
	modGPIOConfigurationRecord config;

//	if (modGPIOInit(&config, (const char *)LEDPORT, pin, kModGPIOOutput))
		xsUnknownError("can't init pin");

	modGPIOWrite(&config, value ? 1 : 0);
	modGPIOUninit(&config);
}

/*
	PWM
*/

void xs_pwm_write(xsMachine *the)
{
#if 0
	int pin = xsToInteger(xsArg(0));
	int value = xsToInteger(xsArg(1));	// 0 to 1023... enforced by modulo in analogWrite implementation
	analogWrite(pin, value);
#endif
}
