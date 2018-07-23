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

/*
	based on Adafruit https://github.com/adafruit/Adafruit_LIS3DH
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "modI2C.h"

#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#define LIS3DH_REG_STATUS1       0x07
#define LIS3DH_REG_OUTADC1_L     0x08
#define LIS3DH_REG_OUTADC1_H     0x09
#define LIS3DH_REG_OUTADC2_L     0x0A
#define LIS3DH_REG_OUTADC2_H     0x0B
#define LIS3DH_REG_OUTADC3_L     0x0C
#define LIS3DH_REG_OUTADC3_H     0x0D
#define LIS3DH_REG_INTCOUNT      0x0E
#define LIS3DH_REG_WHOAMI        0x0F
#define LIS3DH_REG_TEMPCFG       0x1F
#define LIS3DH_REG_CTRL1         0x20
#define LIS3DH_REG_CTRL2         0x21
#define LIS3DH_REG_CTRL3         0x22
#define LIS3DH_REG_CTRL4         0x23
#define LIS3DH_REG_CTRL5         0x24
#define LIS3DH_REG_CTRL6         0x25
#define LIS3DH_REG_REFERENCE     0x26
#define LIS3DH_REG_STATUS2       0x27
#define LIS3DH_REG_OUT_X_L       0x28
#define LIS3DH_REG_OUT_X_H       0x29
#define LIS3DH_REG_OUT_Y_L       0x2A
#define LIS3DH_REG_OUT_Y_H       0x2B
#define LIS3DH_REG_OUT_Z_L       0x2C
#define LIS3DH_REG_OUT_Z_H       0x2D
#define LIS3DH_REG_FIFOCTRL      0x2E
#define LIS3DH_REG_FIFOSRC       0x2F
#define LIS3DH_REG_INT1CFG       0x30
#define LIS3DH_REG_INT1SRC       0x31
#define LIS3DH_REG_INT1THS       0x32
#define LIS3DH_REG_INT1DUR       0x33
#define LIS3DH_REG_CLICKCFG      0x38
#define LIS3DH_REG_CLICKSRC      0x39
#define LIS3DH_REG_CLICKTHS      0x3A
#define LIS3DH_REG_TIMELIMIT     0x3B
#define LIS3DH_REG_TIMELATENCY   0x3C
#define LIS3DH_REG_TIMEWINDOW    0x3D
#define LIS3DH_REG_ACTTHS        0x3E
#define LIS3DH_REG_ACTDUR        0x3F

typedef enum
{
  LIS3DH_RANGE_16_G         = 0b11,   // +/- 16g
  LIS3DH_RANGE_8_G           = 0b10,   // +/- 8g
  LIS3DH_RANGE_4_G           = 0b01,   // +/- 4g
  LIS3DH_RANGE_2_G           = 0b00    // +/- 2g (default value)
} lis3dh_range_t;

/* Used with register 0x2A (LIS3DH_REG_CTRL_REG1) to set bandwidth */
typedef enum
{
  LIS3DH_DATARATE_400_HZ     = 0b0111, //  400Hz 
  LIS3DH_DATARATE_200_HZ     = 0b0110, //  200Hz
  LIS3DH_DATARATE_100_HZ     = 0b0101, //  100Hz
  LIS3DH_DATARATE_50_HZ      = 0b0100, //   50Hz
  LIS3DH_DATARATE_25_HZ      = 0b0011, //   25Hz
  LIS3DH_DATARATE_10_HZ      = 0b0010, // 10 Hz
  LIS3DH_DATARATE_1_HZ       = 0b0001, // 1 Hz
  LIS3DH_DATARATE_POWERDOWN  = 0,
  LIS3DH_DATARATE_LOWPOWER_1K6HZ  = 0b1000,
  LIS3DH_DATARATE_LOWPOWER_5KHZ  =  0b1001,

} lis3dh_dataRate_t;

static void writeRegister8(xsMachine *the, modI2CConfiguration lis3dh, uint8_t reg, uint8_t value);
static uint8_t readRegister8(xsMachine *the, modI2CConfiguration lis3dh, uint8_t reg);

#define kDoStop (1)
#define kNoStop (0)

void xs_lis3dh_destructor(void *data)
{
	if (data) {
		modI2CConfiguration lis3dh = (modI2CConfiguration)data;
		modI2CUninit(lis3dh);
		c_free(data);
	}
}

void xs_lis3dh(xsMachine *the)
{
	modI2CConfiguration lis3dh = c_calloc(1, sizeof(modI2CConfigurationRecord));
	if (!lis3dh)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, lis3dh);

#ifdef MODDEF_LIS3DH_I2C_ADDRESS
	lis3dh->address = MODDEF_LIS3DH_I2C_ADDRESS;
#else
	// retrieve I2C address from dictionary
	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	lis3dh->address = xsmcToInteger(xsVar(0));
#endif
	lis3dh->sda = MODDEF_LIS3DH_I2C_SDA;
	lis3dh->scl = MODDEF_LIS3DH_I2C_SCL;

	modI2CInit(lis3dh);

	if (0x33 != readRegister8(the, lis3dh, LIS3DH_REG_WHOAMI))
		xsUnknownError("unexpected device ID");

	// enable all axes, normal mode @ 400Hz
	writeRegister8(the, lis3dh, LIS3DH_REG_CTRL1, 0x07 | (LIS3DH_DATARATE_400_HZ << 4));

	// High res & BDU enabled
	writeRegister8(the, lis3dh, LIS3DH_REG_CTRL4, 0x88 | (LIS3DH_RANGE_4_G << 4));

	// DRDY on INT1
	writeRegister8(the, lis3dh, LIS3DH_REG_CTRL3, 0x10);

//	// enable adcs
//	writeRegister8(the, LIS3DH_REG_TEMPCFG, 0x80);		//@@ needed??
}

void xs_lis3dh_read(xsMachine *the)
{
	modI2CConfiguration lis3dh = xsmcGetHostData(xsThis);
	uint8_t cmd = LIS3DH_REG_OUT_X_L | 0x80;
	uint8_t values[6];
	int16_t x, y, z;

	if (0 != modI2CWrite(lis3dh, &cmd, sizeof(cmd), kNoStop))
		xsUnknownError("unable to write register");

	if (0 != modI2CRead(lis3dh, values, sizeof(values), kDoStop))
		xsUnknownError("unable to read value");

	x = values[0] | (values[1] << 8);
	y = values[2] | (values[3] << 8);
	z = values[4] | (values[5] << 8);

//@@ don't need to read this each time? and recalculate range
	uint8_t range = (readRegister8(the, lis3dh, LIS3DH_REG_CTRL4) >> 4) & 0x03;
	uint16_t divider = 1;
	if (range == LIS3DH_RANGE_16_G)
		divider = 1365; // different sensitivity at 16g
	else if (range == LIS3DH_RANGE_8_G)
		divider = 4096;
	else if (range == LIS3DH_RANGE_4_G)
		divider = 8190;
	else
		divider = 16380;

	if (xsmcArgc)
		xsResult = xsArg(0);
	else
		xsResult = xsmcNewObject();

	xsmcVars(1);
	xsmcSetNumber(xsVar(0), ((xsNumberValue)x) / (xsNumberValue)divider);
	xsmcSet(xsResult, xsID_x, xsVar(0));

	xsmcSetNumber(xsVar(0), ((xsNumberValue)y) / (xsNumberValue)divider);
	xsmcSet(xsResult, xsID_y, xsVar(0));

	xsmcSetNumber(xsVar(0), ((xsNumberValue)z) / (xsNumberValue)divider);
	xsmcSet(xsResult, xsID_z, xsVar(0));
}

void writeRegister8(xsMachine *the, modI2CConfiguration lis3dh, uint8_t reg, uint8_t value)
{
	uint8_t cmd[2] = {reg, value};

	if (0 != modI2CWrite(lis3dh, cmd, sizeof(cmd), kDoStop))
		xsUnknownError("unable to write register value");
}

uint8_t readRegister8(xsMachine *the, modI2CConfiguration lis3dh, uint8_t reg)
{
	uint8_t cmd;

	if (0 != modI2CWrite(lis3dh, &reg, 1, kNoStop))
		xsUnknownError("unable to write register address");

	if (0 != modI2CRead(lis3dh, &cmd, 1, kDoStop))
		xsUnknownError("unable to read value");

	return cmd;
}
