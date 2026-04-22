/*
 * Copyright (c) 2016-2026 Moddable Tech, Inc.
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

#include "mc.defines.h"
#include "modI2C.h"
#include "xsHost.h"

#include "driver/i2c_master.h"

#if !defined(MODDEF_I2C_PULLUPS)
	#define MODDEF_I2C_PULLUPS	1
#endif

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

__attribute__((weak)) uint8_t i2cActivate(void *);	// ECMA-419 I2C
static uint8_t modI2CActivate(modI2CConfiguration config);

static uint32_t gHz;
static uint16_t gData;
static uint16_t gClock;
static uint8_t gAddress;

static i2c_master_bus_handle_t gBus;
static i2c_master_dev_handle_t gDevice;

void modI2CInit(modI2CConfiguration config)
{
}

void modI2CUninit(modI2CConfiguration config)
{
	if (gDevice)
		i2c_master_bus_rm_device(gDevice);
	if (gBus)
		i2c_del_master_bus(gBus);
	gBus = C_NULL;
	gDevice = C_NULL;
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	if (modI2CActivate(config))
		return 1;

//@@ ignoring sendStop
	int ret = i2c_master_receive(gDevice, buffer, length, config->timeout);

	return (ESP_OK == ret) ? 0 : 1;
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	if (modI2CActivate(config))
		return 1;

//@@ ignoring sendStop
	int ret = i2c_master_transmit(gDevice, buffer, length, config->timeout);

	return (ESP_OK == ret) ? 0 : 1;
}

uint8_t modI2CActivate(modI2CConfiguration config)
{
	int data, clock, hz;

	if (i2cActivate)
		i2cActivate(C_NULL);		// make ECMA-419 release bus

#if defined(MODDEF_I2C_SDA_PIN) && defined(MODDEF_I2C_SCL_PIN)
	data = (-1 == config->sda) ? MODDEF_I2C_SDA_PIN : config->sda;
	clock = (-1 == config->scl) ? MODDEF_I2C_SCL_PIN : config->scl;
#else
	if ((-1 == config->sda) || (-1 == config->scl))
		return 1;

	data = config->sda;
	clock = config->scl;
#endif
	hz = config->hz ? config->hz : 100000;

	if (gBus && ((data != gData) || (clock != gClock)))
		modI2CUninit(config);
	
	if (gDevice && ((hz != gHz) || (config->address != gAddress))) {
		i2c_master_bus_rm_device(gDevice);
		gDevice = C_NULL;
	}

	if (C_NULL == gBus) {
		i2c_master_bus_config_t busC = {
#ifdef MODDEF_I2C_PORT
			.i2c_port = MODDEF_I2C_PORT,
#else
			.i2c_port = -1,
#endif
			.sda_io_num = data,
			.scl_io_num = clock,
			.clk_source = I2C_CLK_SRC_DEFAULT
		};
#if MODDEF_I2C_PULLUPS
		busC.flags.enable_internal_pullup = 1;
#else
		busC.flags.enable_internal_pullup = 0;
#endif
	if (ESP_OK != i2c_new_master_bus(&busC, &gBus))
			return 1;
		
		gClock = clock;
		gData = data;
	}
	
	if (C_NULL == gDevice) {
		i2c_device_config_t deviceC = {
			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
			.device_address = config->address,
			.scl_speed_hz = hz,
		};
		if (ESP_OK != i2c_master_bus_add_device(gBus, &deviceC, &gDevice))
			return 1;

		gAddress = config->address;
		gHz = hz;
	}

	if (config->timeout == 0)
		config->timeout = 400;

	return 0;
}
