/*
	Analog - uing ESP8266
*/

#include "user_interface.h"	// esp8266 functions

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsesp.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

static uint8_t gInUse;

void xs_analog_constructor(xsMachine *the)
{
	int pin;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));
	if (0 != pin)
		xsRangeError("invalid pin");

	if (gInUse)
		xsUnknownError("in use");

	gInUse = 1;
}

void xs_analog_destructor(void *data)
{
	gInUse = 0;
}

void xs_analog_close(xsMachine *the)
{
	xs_analog_destructor(NULL);
}

void xs_analog_read(xsMachine *the)
{
	int value = system_adc_read();
	xsmcSetInteger(xsResult, (value > 1023) ? 1023 : value);		// pin, as API returns 1024.
}

/*
	Normatively define values returned so that they are consistent across microcontrollers from various vendors
*/


