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

#include "xsmc.h"
#include "modI2C.h"

#define MCP23008_ADDRESS 0x20

// registers
#define MCP23008_IODIR 0x00
#define MCP23008_IPOL 0x01
#define MCP23008_GPINTEN 0x02
#define MCP23008_DEFVAL 0x03
#define MCP23008_INTCON 0x04
#define MCP23008_IOCON 0x05
#define MCP23008_GPPU 0x06
#define MCP23008_INTF 0x07
#define MCP23008_INTCAP 0x08
#define MCP23008_GPIO 0x09
#define MCP23008_OLAT 0x0A

static modI2CConfigurationRecord gMCP23008 = {
	0,					// some speed
	4,					// sda
	5,					// scl
	MCP23008_ADDRESS,	// i2c address

};

static uint8_t gInited = 0;
static uint8_t gInputs = 0xFF;		// 1 = input, 0 = output

#define kDoStop (1)
#define kNoStop (0)

static void mcp23008Init(xsMachine *the)
{
	uint8_t initCmd[] = {MCP23008_IODIR, gInputs, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	modI2CInit(&gMCP23008);

	if (0 != modI2CWrite(&gMCP23008, initCmd, sizeof(initCmd), kDoStop))
		xsUnknownError("unable to initialize gMCP23008");

	gInited = 1;
}

void xs_mcp23008_read(xsMachine *the)
{
	xsIntegerValue pin = xsmcToInteger(xsArg(0));
	uint8_t pinMask = 1 << pin;

	uint8_t cmd[2];

	if ((pin < 0) || (pin > 7))
		xsUnknownError("invalid pin");

	if (!gInited)
		mcp23008Init(the);

	if (!(gInputs & pinMask)) {
		// make it an input
		gInputs |= pinMask;

		cmd[0] = MCP23008_IODIR;
		cmd[1] = gInputs;

		if (0 != modI2CWrite(&gMCP23008, cmd, 2, kDoStop))
			xsUnknownError("unable to set pin to input");
	}

	cmd[0] = MCP23008_GPIO;
	if (0 != modI2CWrite(&gMCP23008, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead(&gMCP23008, cmd, 1, kDoStop))
		xsUnknownError("unable to read pins");

	xsmcSetInteger(xsResult, (cmd[0] & pinMask) ? 1 : 0);
}

void xs_mcp23008_write(xsMachine *the)
{
	xsIntegerValue pin = xsmcToInteger(xsArg(0));
	xsIntegerValue value = xsmcToInteger(xsArg(1));
	uint8_t pinMask = 1 << pin;
	uint8_t values;
	uint8_t cmd[2];

	if ((pin < 0) || (pin > 7))
		xsUnknownError("invalid pin");

	if (!gInited)
		mcp23008Init(the);

	if (gInputs & pinMask) {
		// make it an output
		gInputs &= ~pinMask;

		cmd[0] = MCP23008_IODIR;
		cmd[1] = gInputs;
		if (0 != modI2CWrite(&gMCP23008, cmd, 2, kDoStop))
			xsUnknownError("unable to set pin to output");
	}

	cmd[0] = MCP23008_GPIO;
	if (0 != modI2CWrite(&gMCP23008, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead(&gMCP23008, &values, 1, kDoStop))
		xsUnknownError("unable to read pins");

	if (value)
		values |= pinMask;
	else
		values &= ~pinMask;

	cmd[0] = MCP23008_GPIO;
	if (0 != modI2CWrite(&gMCP23008, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CWrite(&gMCP23008, &values, 1, kDoStop))
		xsUnknownError("unable to write pins");
}
