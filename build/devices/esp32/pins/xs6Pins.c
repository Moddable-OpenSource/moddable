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

#include "xs.h"
#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

#include "modGPIO.h"
#include "modI2C.h"

#include "driver/adc.h"

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

	if (modGPIOInit(&config, NULL, pin, kModGPIOInput))
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

	if (modGPIOInit(&config, NULL, pin, kModGPIOOutput))
		xsUnknownError("can't init pin");

	modGPIOWrite(&config, value ? 1 : 0);
	modGPIOUninit(&config);
}

/*
	I2C
*/

void xs_i2c(xsMachine *the)
{
	modI2CConfiguration i2c;
	int hz = 0;
	int sda = xsToInteger(xsGet(xsArg(0), xsID_sda));
	int scl = xsToInteger(xsGet(xsArg(0), xsID_clock));
	int address = xsToInteger(xsGet(xsArg(0), xsID_address));
	if ((address < 0) || (address > 127))
		xsUnknownError("invalid address");

	if (xsHas(xsArg(0), xsID_hz))
		hz = xsToInteger(xsGet(xsArg(0), xsID_hz));

	i2c = xsSetHostChunk(xsThis, NULL, sizeof(modI2CConfigurationRecord));

	i2c->hz = hz;
	i2c->sda = sda;
	i2c->scl = scl;
	i2c->address = address;
	modI2CInit(i2c);
}

void xs_i2c_destructor(void *data)
{
	if (data)
		modI2CUninit((modI2CConfiguration)data);
}

void xs_i2c_read(xsMachine *the)
{
	modI2CConfiguration i2c;
	unsigned int len = xsToInteger(xsArg(0)), i;
	unsigned char err;
	unsigned char buffer[34];

	if (len > sizeof(buffer))
		xsUnknownError("34 byte read limit");

	i2c = xsGetHostChunk(xsThis);
	err = modI2CRead(i2c, buffer, len, 1);
	if (err)
		return;		// undefined returned on read failure

	xsResult = xsArrayBuffer(buffer, len);
	xsResult = xsNew1(xsGlobal, xsID_Uint8Array, xsResult);
}

void xs_i2c_write(xsMachine *the)
{
	modI2CConfiguration i2c;
	uint8_t argc = xsToInteger(xsArgc), i;
	unsigned char err;
	unsigned int len = 0;
	unsigned char buffer[34];

	for (i = 0; i < argc; i++) {
		xsType t = xsTypeOf(xsArg(i));
		if ((xsNumberType == t) || (xsIntegerType == t)) {
			if ((len + 1) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			buffer[len++] = (unsigned char)xsToInteger(xsArg(i));
			continue;
		}
		if (xsStringType == t) {
			char *s = xsToString(xsArg(i));
			int l = espStrLen(s);
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			espMemCpy(buffer + len, s, l);
			len += l;
			continue;
		}

		{	// assume some kind of array (Array, Uint8Array, etc)
			int l = xsToInteger(xsGet(xsArg(i), xsID_length));
			uint8_t i;
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			for (i = 0; i < l; i++)
				buffer[len++] = xsToInteger(xsGet(xsArg(i), i));
		}
	}

	i2c = xsGetHostChunk(xsThis);
	err = modI2CWrite(i2c, buffer, len, 1);
	if (err)
		xsUnknownError("write failed");
}

/*
	Analog
*/

void xs_analog_read(xsMachine *the)
{
	static uint8_t inited = 0;
	int pin = xsToInteger(xsArg(0));
	int channel, value;

	if (!inited) {
		int i;

		inited = 1;
		adc1_config_width(ADC_WIDTH_10Bit);
		for (i = ADC1_CHANNEL_0; i <= ADC1_CHANNEL_7; i++)
			adc1_config_channel_atten(i, ADC_ATTEN_0db);
	}

	switch (pin) {
		case 36: channel = ADC1_CHANNEL_0; break;
		case 37: channel = ADC1_CHANNEL_1; break;
		case 38: channel = ADC1_CHANNEL_2; break;
		case 39: channel = ADC1_CHANNEL_3; break;
		case 32: channel = ADC1_CHANNEL_4; break;
		case 33: channel = ADC1_CHANNEL_5; break;
		case 34: channel = ADC1_CHANNEL_6; break;
		case 35: channel = ADC1_CHANNEL_7; break;
		default:  xsUnknownError("invalid analog pin"); break;

	}

	value = adc1_get_voltage(channel);		// 0 to 1023
	xsResult = xsInteger(value);
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
