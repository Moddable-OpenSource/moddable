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
#include "bmp280.h"

#ifndef MODDEF_BMP280_SDA_PIN
	#define MODDEF_BMP280_SDA_PIN MODDEF_I2C_SDA_PIN
#endif
#ifndef MODDEF_BMP280_SCL_PIN
	#define MODDEF_BMP280_SCL_PIN MODDEF_I2C_SCL_PIN
#endif

#define BMP_REG_ADDR_ID	0xD0

void geckoEM1Idle(uint32_t ms);

#define NUM_I2C_TRIES	10
struct BMP280Record {
	modI2CConfigurationRecord i2c;
	struct bmp280_t bmp280Driver;
};
typedef struct BMP280Record BMP280Record;
typedef BMP280Record *BMP280;

static BMP280 gBmp = NULL;
static uint8_t gBmpUsers = 0;
static uint8_t powerMode = BMP280_FORCED_MODE;

static int8_t  i2cBusRead   ( uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count );
static int8_t  i2cBusWrite  ( uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count );

int8_t i2cBusRead(uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count ) {
	I2C_TransferReturn_TypeDef err;
	err = modI2CWriteRead(&gBmp->i2c, &regAddr, 1, regData, count);
	return err;
}
int8_t i2cBusWrite ( uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count ) {
	I2C_TransferReturn_TypeDef err;
	err = modI2CWriteWrite(&gBmp->i2c, &regAddr, 1, regData, count);
	return err;
}


void xs_BMP280_destructor(void *data)
{
	if (0 == (--gBmpUsers)) {
		bmp280_set_power_mode(BMP280_SLEEP_MODE);
	    GPIO_PinOutClear(MODDEF_BMP280_ENABLE_PORT, MODDEF_BMP280_ENABLE_PIN);
		modI2CUninit(&gBmp->i2c);
		gBmp = NULL;
	}
}

void xs_BMP280(xsMachine *the)
{
	int result;
	uint8_t cmd = BMP_REG_ADDR_ID;
	uint8_t data;
	I2C_TransferReturn_TypeDef err;
	int i;

	if (!gBmp) {
		gBmp = c_calloc(1, sizeof(BMP280Record));
		if (!gBmp) xsUnknownError("out of memory");

		GPIO_PinModeSet(MODDEF_BMP280_ENABLE_PORT, MODDEF_BMP280_ENABLE_PIN, gpioModePushPull, 1);

		xsmcVars(1);
		gBmp->i2c.sda = MODDEF_BMP280_SDA_PIN;
		gBmp->i2c.scl = MODDEF_BMP280_SCL_PIN;
		gBmp->i2c.address = MODDEF_BMP280_ADDR;
		gBmp->i2c.hz = MODDEF_BMP280_HZ;
		modI2CInit(&gBmp->i2c);
	}
	gBmpUsers++;

	for (i=0; i<NUM_I2C_TRIES; i++) {
		err = modI2CWriteRead(&gBmp->i2c, &cmd, 1, &data, 1);
		if (0 == err) break;
		geckoEM1Idle(1);
	}
	if (err) xsUnknownError("getting BMP280_DEVICE_ID failed");
	if (data != MODDEF_BMP280_DEVICE_ID)
		xsUnknownError("wrong device id");

	gBmp->bmp280Driver.bus_write = i2cBusWrite;
	gBmp->bmp280Driver.bus_read = i2cBusRead;
	gBmp->bmp280Driver.dev_addr = MODDEF_BMP280_DEVICE_ID;
	gBmp->bmp280Driver.delay_msec = gecko_delay;

	result = bmp280_init(&gBmp->bmp280Driver);
	if (result)
		xsUnknownError("bmp280_init failed");
	result = bmp280_set_work_mode(BMP280_ULTRA_HIGH_RESOLUTION_MODE);
	if (result)
		xsUnknownError("bmp280_set_work_mode failed");

}

// pressure in milli-bar
// temperature in milli-degrees celcius

void xs_BMP280_read(xsMachine *the) {
	float temp, pressure;
	int8_t result;
	int32_t uncompTemp;
	int32_t uncompPressure;
	int32_t compTemp;

	xsmcVars(2);

	if (powerMode == BMP280_NORMAL_MODE) {
		result = bmp280_read_uncomp_pressure(&uncompPressure);
		if (!result)
			result = bmp280_read_uncomp_temperature(&uncompTemp);
	}
	else
		result = bmp280_get_forced_uncomp_pressure_temperature(&uncompPressure, &uncompTemp);

	if (result)
		xsUnknownError("bmp280: unknown i2c read error");

	compTemp = bmp280_compensate_temperature_int32(uncompTemp);
	temp = (float)compTemp / 100.0f;
	pressure = (float)bmp280_compensate_pressure_int64(uncompPressure);
	pressure /= 25600.0f;

	xsmcSetNumber(xsVar(1), pressure);
	xsmcSet(xsArg(0), xsID_millibar, xsVar(1));
	xsmcSetNumber(xsVar(1), temp);
	xsmcSet(xsArg(0), xsID_celcius, xsVar(1));
}

