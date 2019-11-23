/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "mc.xs.h"
#include "xsHost.h"
#include "modGPIO.h"
#include "modI2C.h"

#ifndef MODDEF_SI7021_SDA_PIN
	#define MODDEF_SI7021_SDA_PIN MODDEF_I2C_SDA_PIN
#endif
#ifndef MODDEF_SI7021_SCL_PIN
	#define MODDEF_SI7021_SCL_PIN MODDEF_I2C_SCL_PIN
#endif

#define SI7021_CMD_MEASURE_RH_NO_HOLD    0xF5            /**< Measure Relative Humidity, No Hold Master Mode */
#define SI7021_CMD_READ_TEMP             0xE0            /**< Read Temperature Value from Previous RH Measurement */
#define SI7021_CMD_READ_ID_BYTE2         {0xFC, 0xC9}    /**< Read Electronic ID 2nd Byte */

void geckoEM1Idle(uint32_t ms);

#define NUM_I2C_TRIES	10
struct si7021Record {
	modI2CConfigurationRecord i2c;
};
typedef struct si7021Record si7021Record;
typedef si7021Record *si7021;

void xs_SI7021_destructor(void *data)
{
    GPIO_PinOutClear(MODDEF_SI7021_ENABLE_PORT, MODDEF_SI7021_ENABLE_PIN);
	if (data) {
		si7021 si = data;
		modI2CUninit(&si->i2c);
		c_free(data);
	}
}

void xs_SI7021(xsMachine *the)
{
	si7021 si;
	uint8_t cmd[2] = SI7021_CMD_READ_ID_BYTE2;
	uint8_t data[8];
	I2C_TransferReturn_TypeDef err;
int i;

	si = c_calloc(1, sizeof(si7021Record));
	if (!si) xsUnknownError("out of memory");
	xsmcSetHostData(xsThis, si);

	GPIO_PinModeSet(MODDEF_SI7021_ENABLE_PORT, MODDEF_SI7021_ENABLE_PIN, gpioModePushPull, 1);

	xsmcVars(1);
	si->i2c.sda = MODDEF_SI7021_SDA_PIN;
	si->i2c.scl = MODDEF_SI7021_SCL_PIN;
	si->i2c.address = MODDEF_SI7021_ADDR;
	si->i2c.hz = MODDEF_SI7021_HZ;
	modI2CInit(&si->i2c);

	for (i=0; i<NUM_I2C_TRIES; i++) {
		err = modI2CWriteRead(&si->i2c, cmd, 2, data, 8);
		if (0 == err) break;
		geckoEM1Idle(10);
	}
	if (err) xsUnknownError("getting SI7021_CMD_READ_ID_BYTE2 failed");
	if (data[0] != MODDEF_SI7021_DEVICE_ID)
		xsUnknownError("wrong device id");
}

void xs_SI7021_read(xsMachine *the) {
	si7021 si = (si7021)xsmcGetHostData(xsThis);
	I2C_TransferReturn_TypeDef err;
	uint8_t cmd, i;
	uint8_t readData[2];
	uint32_t rhData;
	int32_t tData;
	float x;

	xsmcVars(2);

	// relative humidity in milli-percent
	// temperature in milli-degrees celsius
	cmd = SI7021_CMD_MEASURE_RH_NO_HOLD;
	err = modI2CWrite(&si->i2c, &cmd, 1, true);
	if (err) xsUnknownError("measure RH_NO_HOLD failed");

	for (i=0; i<NUM_I2C_TRIES; i++) {
		err = modI2CRead(&si->i2c, readData, 2, true);
		if (0 == err) {
			rhData = ((uint32_t)readData[0] << 8) + (readData[1] & 0xfc);
			rhData = (((rhData) * 15625L) >> 13) - 6000;
			break;
		}
		else if ((i2cTransferInProgress == err) || (i2cTransferNack == err)) {
			geckoEM1Idle(10);
		}
		else {
			xsUnknownError("SI7021: unknown i2c read error");
		}
	}
	if (i == NUM_I2C_TRIES)
		return;

	cmd = SI7021_CMD_READ_TEMP;
	err = modI2CWriteRead(&si->i2c, &cmd, 1, readData, 2);
	if (err) xsUnknownError("measure READ_TEMP failed");
	tData = ((uint32_t)readData[0] << 8) + (readData[1] & 0xfc);
	tData = (((tData) * 21965L) >> 13) - 46850;

	x = (float)rhData / 1000.0;
	xsmcSetNumber(xsVar(1), x);
	xsmcSet(xsArg(0), xsID_humidity, xsVar(1));
	x = (float)tData / 1000.0;
	xsmcSetNumber(xsVar(1), x);
	xsmcSet(xsArg(0), xsID_celsius, xsVar(1));
}

