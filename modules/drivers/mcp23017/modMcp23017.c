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

#include "xsPlatform.h"
#include "xsmc.h"
#include "modI2C.h"
#include "modGPIO.h"

#include "mc.xs.h"
#include "mc.defines.h"

// #define MCP23017_ADDRESS 0x20

// registers
#define MCP23017_IODIRA		0x00
#define MCP23017_IODIRB		0x01
#define MCP23017_IPOLA		0x02
#define MCP23017_IPOLB		0x03
#define MCP23017_GPINTENA	0x04
#define MCP23017_GPINTENB	0x05
#define MCP23017_DEFVALA	0x06
#define MCP23017_DEFVALB	0x07
#define MCP23017_INTCONA	0x08
#define MCP23017_INTCONB	0x09
#define MCP23017_IOCONA		0x0A
#define MCP23017_IOCONB		0x0B
#define MCP23017_GPPUA		0x0C
#define MCP23017_GPPUB		0x0D
#define MCP23017_INTFA		0x0E
#define MCP23017_INTFB		0x0F
#define MCP23017_INTCAPA	0x10
#define MCP23017_INTCAPB	0x11
#define MCP23017_GPIOA		0x12
#define MCP23017_GPIOB		0x13
#define MCP23017_OLATA		0x14
#define MCP23017_OLATB		0x15

static uint8_t initCmd[] = { MCP23017_IODIRA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

typedef struct mcp23017ConfigurationRecord {
	modI2CConfigurationRecord	config;

	uint8_t inputsA;	// bit array: 1 = input, 0 = output
	uint8_t inputsB;	// bit array: 1 = input, 0 = output
	uint8_t pullupA;	// bit array: 1 = 100k pull up for inputs
	uint8_t pullupB;	// bit array: 1 = 100k pull up for inputs
} mcp23017ConfigurationRecord, *mcp23017Configuration;


#define kDoStop (1)
#define kNoStop (0)

#if 1
#define dumpRegisters(a,b)
#define dumpRegistersR(a,b)
#define dumpRegistersW(a,b)
#else
#define dumpRegistersW(a,b) dumpRegisters(a,b)
#define dumpRegistersR(a,b) dumpRegisters(a,b)

static void bitStr(uint8_t val, char *str) {
	int i;
	for (i=0; i<8; i++) {
		*str++ = val & 0x80	? '1' : '0';
		val <<= 1;
	}
	*str = '\0';
}

static void dumpRegisters(xsMachine *the, char *str) {
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	uint8_t cmd[2];
	int i;
	char outStr[256], bitstr[10], endChar, *descStr;

	snprintf(outStr, 256, "%s\n", str);

	for (i=	MCP23017_IODIRA; i<= MCP23017_GPIOB; i++) {
		switch(i) {
			case MCP23017_IODIRA:
				descStr = "IODIRA:";
				endChar = ' ';
				break;
			case MCP23017_IODIRB:
				descStr = "B:";
				endChar = '\n';
				break;
			case MCP23017_GPIOA:
				descStr = " GPIOA:";
				endChar = ' ';
				break;
			case MCP23017_GPIOB:
				descStr = "B:";
				endChar = '\n';
				break;
			case MCP23017_GPPUA:
				descStr = " GPPUA:";
				endChar = ' ';
				break;
			case MCP23017_GPPUB:
				descStr = "B:";
				endChar = '\n';
				break;
			default:
				continue;
		}
		cmd[0] = i;
		if (0 != modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, cmd, 1))
			xsUnknownError("unable to send gpio register or read pins");
		bitStr(cmd[0], bitstr);
		snprintf(outStr, 256, "%s%s(%03d) %s%c", outStr, descStr, cmd[0], bitstr, endChar);
	}
	xsTrace(outStr);
}
#endif

void xs_mcp23017_destructor(void *data) {
	if (data) {
		modI2CUninit((modI2CConfiguration)data);
		c_free(data);
	}
}

static void doI2Cwrite(xsMachine *the, mcp23017Configuration mcp23017, uint8_t reg, uint16_t val, uint8_t size) {
	uint8_t cmd[3];
	cmd[0] = reg;
	if (size > 1) {
		cmd[1] = val & 0xff;
		cmd[2] = val & (0xff00) >> 8;
	}
	else
		cmd[1] = val;
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, size > 1 ? 3 : 2, kDoStop))
		xsUnknownError("i2c err");
}

void xs_mcp23017(xsMachine *the)
{
	mcp23017Configuration mcp23017 = c_calloc(1, sizeof(mcp23017ConfigurationRecord));
	uint16_t set;

	if (!mcp23017)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, mcp23017);

	xsmcVars(1);
#ifdef MODDEF_MCP23017_I2C_ADDRESS
	mcp23017->config.address = MODDEF_MCP23017_I2C_ADDRESS;
#else
	// retrieve I2C address from dictionary
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	mcp23017->config.address = xsmcToInteger(xsVar(0));
#endif
#ifdef MODDEF_MCP23017_I2C_HZ
	mcp23017->config.hz = MODDEF_MCP23017_I2C_HZ;
#else
	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	mcp23017->config.hz = xsmcToInteger(xsVar(0));
#endif

	modI2CInit((modI2CConfiguration)mcp23017);

	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, initCmd, sizeof(initCmd), kDoStop))
		xsUnknownError("unable to initialize MCP23017");

	if (xsmcHas(xsArg(0), xsID_inputs)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_inputs);
		set = xsmcToInteger(xsVar(0));
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, set, 2);
	}

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		set = xsmcToInteger(xsVar(0));
		doI2Cwrite(the, mcp23017, MCP23017_GPPUA, set, 2);
	}

	dumpRegisters(the, "afterInit:");
}

void xs_mcp23017_configure(xsMachine *the) {
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pin = xsmcToInteger(xsArg(0));
	xsIntegerValue mode = xsmcToInteger(xsArg(1));
	uint16_t pinMask = 1 << pin;
	uint8_t isInput = 0, doPull = 0, doDir = 0;
	
	if ((pin < 0) || (pin > 15))
		xsUnknownError("mcp23017 configure: invalid pin %d", pin);

	switch (mode) {
		case kModGPIOInput:
		case kModGPIOInputPullUp:
			isInput = 1;
			break;
		case kModGPIOOutput:
			break;
		default:
			xsUnknownError("configure: bad mode");
	}

	dumpRegistersW(the, "beginConfigure-:");

	if (pin < 8) {			// portA
		if (!isInput) {
			// make it an output
			if (mcp23017->inputsA & pinMask) {
				mcp23017->inputsA &= ~pinMask;
				doDir = 1;
			}
		}
		else {
			if (!(mcp23017->inputsA & pinMask)) {
				mcp23017->inputsA |= pinMask;
				doDir = 1;
			}
			if (mode == kModGPIOInputPullUp) {
				if (!(mcp23017->pullupA & pinMask)) {
					mcp23017->pullupA |= pinMask;
					doPull = 1;
				}
			}
			else {	// remove pullup
				if (mcp23017->pullupA & pinMask) {
					mcp23017->pullupA &= ~pinMask;
					doPull = 1;
				}

			}
		}
		if (doDir)
			doI2Cwrite(the, mcp23017, MCP23017_IODIRA, mcp23017->inputsA, 1);
		if (doPull)
			doI2Cwrite(the, mcp23017, MCP23017_GPPUA, mcp23017->pullupA, 1);
	}
	else {					// portB
		pinMask >>= 8;
		if (!isInput) {
			// make it an output
			if (mcp23017->inputsB & pinMask) {
				mcp23017->inputsB &= ~pinMask;
				doDir = 1;
			}
		}
		else {
			if (!(mcp23017->inputsB & pinMask)) {
				mcp23017->inputsB |= pinMask;
				doDir = 1;
			}
			if (mode == kModGPIOInputPullUp) {
				if (!(mcp23017->pullupB & pinMask)) {
					mcp23017->pullupB |= pinMask;
					doPull = 1;
				}
			}
		}
		if (doDir)
			doI2Cwrite(the, mcp23017, MCP23017_IODIRB, mcp23017->inputsB, 1);
		if (doPull)
			doI2Cwrite(the, mcp23017, MCP23017_GPPUB, mcp23017->pullupB, 1);
	}

	dumpRegisters(the, "after configure:");
}

void xs_mcp23017_configure_set(xsMachine *the) {
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pinMask = xsmcToInteger(xsArg(0));
	xsIntegerValue mode = xsmcToInteger(xsArg(1));
	uint8_t pinMaskA = pinMask & 0xff;
	uint8_t pinMaskB = (pinMask & 0xff00) >> 8;
	uint8_t isInput = 0, doPull = 0, doDir = 0;
	uint8_t nuMask;
	
	switch (mode) {
		case kModGPIOInput:
		case kModGPIOInputPullUp:
			isInput = 1;
			break;
		case kModGPIOOutput:
			break;
		default:
			xsUnknownError("configure: bad mode");
	}

	dumpRegistersW(the, "beginConfigureSet-:");

	if (pinMaskA) {					// portA
		if (!isInput) {
			// make it an output
			nuMask = (mcp23017->inputsA & ~pinMaskA);
			if (mcp23017->inputsA != nuMask) {
				mcp23017->inputsA = nuMask;
				doDir = 1;
			}
		}
		else {
			nuMask = mcp23017->inputsA | pinMaskA;
			if (mcp23017->inputsA != nuMask) {
				mcp23017->inputsA |= nuMask;
				doDir = 1;
			}
			if (mode == kModGPIOInputPullUp) {
				if (!(mcp23017->pullupA & pinMask)) {
					mcp23017->pullupA |= pinMaskA;
					doPull = 1;
				}
			}
			else {	// remove pullup
				nuMask = mcp23017->pullupA & ~pinMaskA;
				if (mcp23017->pullupA != nuMask) {
					mcp23017->pullupA = nuMask;
					doPull = 1;
				}
			}
		}
	}
	if (pinMaskB) {					// portB
		if (!isInput) {
			// make it an output
			nuMask = (mcp23017->inputsB & ~pinMaskB);
			if (mcp23017->inputsB != nuMask) {
				mcp23017->inputsB = nuMask;
				doDir = 1;
			}
		}
		else {
			nuMask = mcp23017->inputsB | pinMaskB;
			if (mcp23017->inputsB != nuMask) {
				mcp23017->inputsB |= nuMask;
				doDir = 1;
			}
			if (mode == kModGPIOInputPullUp) {
				if (!(mcp23017->pullupB & pinMask)) {
					mcp23017->pullupB |= pinMaskB;
					doPull = 1;
				}
			}
			else {	// remove pullup
				nuMask = mcp23017->pullupB & ~pinMaskB;
				if (mcp23017->pullupB != nuMask) {
					mcp23017->pullupB = nuMask;
					doPull = 1;
				}
			}
		}
	}

	if (doDir)
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, (mcp23017->inputsB << 8) | mcp23017->inputsA, 1);
	if (doPull)
		doI2Cwrite(the, mcp23017, MCP23017_GPPUA, (mcp23017->pullupB << 8) | mcp23017->inputsA, 1);

	dumpRegisters(the, "after configure_set:");
}

void xs_mcp23017_read(xsMachine *the)
{
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pin = xsmcToInteger(xsArg(0));
	uint16_t result, pinMask = 1 << pin;
	uint8_t pinMaskA, pinMaskB, doit=0;

	uint8_t cmd[2];

	if ((pin < 0) || (pin > 15))
		xsUnknownError("invalid pin");

/* MDK
	can just do one iteration since we know if it is A or B based on "pin"
*/

	pinMaskA = pinMask & 0xff;
	pinMaskB = (pinMask >> 8) & 0xff;
	if (!(mcp23017->inputsA & pinMaskA)) {
		mcp23017->inputsA |= pinMaskA;
		doit = 1;
	}
	if (!(mcp23017->inputsB & pinMaskB)) {
		mcp23017->inputsB |= pinMaskB;
		doit = 1;
	}
	if (doit)
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, mcp23017->inputsA | (mcp23017->inputsB << 8), 2);

	if (pin < 8) {
		cmd[0] = MCP23017_GPIOA;
#if gecko
		if (0 != modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, cmd, 1))
			xsUnknownError("unable to send gpio register or read pins");
#else
		if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
			xsUnknownError("unable to send gpio register");

		if (0 != modI2CRead((modI2CConfiguration)mcp23017, cmd, 1, kDoStop))
			xsUnknownError("unable to read pins");
#endif
		result = ((cmd[0] & pinMaskA) ? 1 : 0);
	}
	else {
		cmd[0] = MCP23017_GPIOB;
#if gecko
		if (0 != modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, cmd, 1))
			xsUnknownError("unable to send gpio register or read pins");
#else
		if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
			xsUnknownError("unable to send gpio register");

		if (0 != modI2CRead(mcp23017, cmd, 1, kDoStop))
			xsUnknownError("unable to read pins");
#endif

		result = ((cmd[0] & pinMaskB) ? 1 : 0);
	}
	xsmcSetInteger(xsResult, result);
}

void xs_mcp23017_write(xsMachine *the)
{
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pin = xsmcToInteger(xsArg(0));
	xsIntegerValue value = xsmcToInteger(xsArg(1));
	uint16_t pinMask = 1 << pin;
	uint8_t pinMaskA, pinMaskB;
	uint8_t valuesA = 0, valuesB = 0;
	uint8_t cmd[3], doit = 0;
int err;

/* MDK
	can just do one iteration since we know if it is A or B based on "pin"
*/
	if ((pin < 0) || (pin > 15))
		xsUnknownError("invalid pin %d", pin);

	dumpRegistersW(the, "beginWrite-:");

	pinMaskA = pinMask & 0xff;
	pinMaskB = (pinMask >> 8) & 0xff;
	if (mcp23017->inputsA & pinMaskA) {
		mcp23017->inputsA &= ~pinMaskA;
		doit = 1;
	}
	if (mcp23017->inputsB & pinMaskB) {
		// make it an output
		mcp23017->inputsB &= ~pinMaskB;
		doit = 1;
	}
	if (doit)
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, mcp23017->inputsA | (mcp23017->inputsB << 8), 2);

	dumpRegistersW(the, "after IODIR:");

	cmd[0] = MCP23017_GPIOA;
#if gecko
	if (0 != (err = modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, &valuesA, 1)))
		xsUnknownError("unable to send gpio register or read pins %d", err);
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead((modI2CConfiguration)mcp23017, &valuesA, 1, kDoStop))
		xsUnknownError("unable to read pins");
#endif
	if (value)
		valuesA |= pinMaskA;
	else
		valuesA &= ~pinMaskA;

	dumpRegistersW(the, "after READA:");

	cmd[0] = MCP23017_GPIOB;
#if gecko
	if (0 != (err = modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, &valuesB, 1)))
		xsUnknownError("unable to send gpio register or read pins %d", err);
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead(mcp23017, &valuesB, 1, kDoStop))
		xsUnknownError("unable to read pins");
#endif
	if (value)
		valuesB |= pinMaskB;
	else
		valuesB &= ~pinMaskB;

	dumpRegistersW(the, "after READB:");

	cmd[0] = MCP23017_GPIOA;
#if gecko
	cmd[1] = valuesA;
	cmd[2] = valuesB;
	if (0 != (err = modI2CWrite((modI2CConfiguration)mcp23017, cmd, 3, kDoStop)))
		xsUnknownError("unable to send gpio register or read pins %d", err);
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	cmd[0] = valuesA;
	cmd[1] = valuesB;
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, &cmd, 2, kDoStop))
		xsUnknownError("unable to write pins");
#endif

	dumpRegistersW(the, "after write:");
}

void xs_mcp23017_read_set(xsMachine *the)
{
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pin_set = xsmcToInteger(xsArg(0));
	uint16_t result;
	uint8_t pinMaskA = pin_set & 0xff, pinMaskB = (pin_set >> 8) & 0xff;
	uint8_t nuMask;
	uint8_t cmd[2], doit = 0;

	nuMask = ((mcp23017->inputsA & ~pinMaskA) | pinMaskA);
	if (nuMask != mcp23017->inputsA) {
		// make it an input
		mcp23017->inputsA = nuMask;
		doit = 1;
	}
	nuMask = ((mcp23017->inputsB & ~pinMaskB) | pinMaskB);
	if (nuMask != mcp23017->inputsB) {
		// make it an input
		mcp23017->inputsB = nuMask;
		doit = 1;
	}
	if (doit)
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, mcp23017->inputsA | (mcp23017->inputsB << 8), 2);

	dumpRegistersR(the, "after setting IODIR:");
	cmd[0] = MCP23017_GPIOA;
#if gecko
	if (0 != modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, cmd, 2))
		xsUnknownError("unable to send gpio register or read pins");

	result = ((cmd[1] << 8) | cmd[0]) & pin_set;
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead((modI2CConfiguration)mcp23017, cmd, 1, kDoStop))
		xsUnknownError("unable to read pins");

	result = cmd[0];

	cmd[0] = MCP23017_GPIOB;

	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead(mcp23017, cmd, 1, kDoStop))
		xsUnknownError("unable to read pins");

	result |= (cmd[0] << 8);
	result &= pin_set;
#endif

	xsmcSetInteger(xsResult, result);
}

void xs_mcp23017_write_set(xsMachine *the)
{
	mcp23017Configuration mcp23017 = xsmcGetHostData(xsThis);
	xsIntegerValue pin_set = xsmcToInteger(xsArg(0));
	xsIntegerValue value = xsmcToInteger(xsArg(1));
	uint8_t pinMaskA = pin_set & 0xff, pinMaskB = (pin_set >> 8) & 0xff;
	uint8_t cmd[3], doit = 0;
int err;

	dumpRegistersW(the, "beginWrite-:");

	if (mcp23017->inputsA & pinMaskA) {
		// make it an output
		mcp23017->inputsA &= ~pinMaskA;
		doit = 1;
	}
	if (mcp23017->inputsB & pinMaskB) {
		// make it an output
		mcp23017->inputsB &= ~pinMaskB;
		doit = 1;
	}
	if (doit)
		doI2Cwrite(the, mcp23017, MCP23017_IODIRA, mcp23017->inputsA | (mcp23017->inputsB << 8), 2);

	dumpRegistersW(the, "after IODIR:");

	cmd[0] = MCP23017_GPIOA;
#if gecko
	if (0 != (err = modI2CWriteRead((modI2CConfiguration)mcp23017, cmd, 1, cmd, 2)))
		xsUnknownError("unable to send gpio register or read pins %d", err);
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CRead((modI2CConfiguration)mcp23017, cmd, 2, kDoStop))
		xsUnknownError("unable to read pins");
#endif

	cmd[2] = (cmd[1] & ~pinMaskB) | ((value >> 8) & 0xff);
	cmd[1] = (cmd[0] & ~pinMaskA) | (value & 0xff);
	cmd[0] = MCP23017_GPIOA;

#if gecko
	if (0 != (err = modI2CWrite((modI2CConfiguration)mcp23017, cmd, 3, kDoStop)))
		xsUnknownError("unable to send gpio register or read pins %d", err);
#else
	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, cmd, 1, kNoStop))
		xsUnknownError("unable to send gpio register");

	if (0 != modI2CWrite((modI2CConfiguration)mcp23017, &cmd, 2, kDoStop))
		xsUnknownError("unable to write pins");
#endif

	dumpRegistersW(the, "after write:");
}


