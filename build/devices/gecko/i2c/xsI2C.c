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
#include "modI2C.h"

/*
	I2C
*/

void xs_i2c(xsMachine *the)
{
	modI2CConfiguration i2c;
	int hz = 0;
	int address = xsToInteger(xsGet(xsArg(0), ID(address)));
	if ((address < 0) || (address > 127))
		xsUnknownError("invalid address");

	if (xsHas(xsArg(0), ID(hz)))
		hz = xsToInteger(xsGet(xsArg(0), ID(hz)));

	i2c = xsSetHostChunk(xsThis, NULL, sizeof(modI2CConfigurationRecord));

	i2c->hz = hz;
//	i2c->sda = sda;
//	i2c->scl = scl;
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
	unsigned int len = xsToInteger(xsArg(0));
	unsigned char err;
	unsigned char buffer[34];

	if (len > sizeof(buffer))
		xsUnknownError("34 byte read limit");

	i2c = xsGetHostChunk(xsThis);
	err = modI2CRead(i2c, buffer, len, true);
	if (err)
		return;		// undefined returned on read failure

	xsResult = xsArrayBuffer(buffer, len);
	xsResult = xsNew1(xsGlobal, ID(Uint8Array), xsResult);
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
			int l = c_strlen(s);
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			c_memcpy(buffer + len, s, l);
			len += l;
			continue;
		}

		{	// assume some kind of array (Array, Uint8Array, etc)
			int l = xsToInteger(xsGet(xsArg(i), ID(length)));
			uint8_t i;
			if ((len + l) > sizeof(buffer))
				xsUnknownError("34 byte write limit");
			for (i = 0; i < l; i++)
				buffer[len++] = xsToInteger(xsGet(xsArg(i), i));
		}
	}

	i2c = xsGetHostChunk(xsThis);
	err = modI2CWrite(i2c, buffer, len, true);
	if (err)
		xsUnknownError("write failed");
}

