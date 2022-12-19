/*
 * Copyright (c) 2016-2022 Moddable Tech, Inc.
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

#include "driver/i2c.h"

#if !defined(MODDEF_I2C_PULLUPS)
	#define MODDEF_I2C_PULLUPS	1
#endif

#if !defined(MODDEF_I2C_PORT)
	#define MODDEF_I2C_PORT	I2C_NUM_1
#endif

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static uint8_t modI2CActivate(modI2CConfiguration config);

static uint32_t gHz;		// non-zero when driver initialized
static uint16_t gSda;
static uint16_t gScl;
static uint16_t gTimeout = DEFAULT_ESP32_I2C_TIMEOUT;

void modI2CInit(modI2CConfiguration config)
{
}

void modI2CUninit(modI2CConfiguration config)
{
	if (gHz) {
		i2c_driver_delete(MODDEF_I2C_PORT);
		gHz = 0;
	}
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;
	i2c_cmd_handle_t cmd;

	if (modI2CActivate(config))
		return 1;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (config->address << 1) | I2C_MASTER_READ, 1);
	if (length > 1)
		i2c_master_read(cmd, buffer, length - 1, 0);
	i2c_master_read(cmd, buffer + length - 1, 1, 1);
	if (sendStop)
		i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(MODDEF_I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return (ESP_OK == ret) ? 0 : 1;
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;
	i2c_cmd_handle_t cmd;

	if (modI2CActivate(config))
		return 1;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (config->address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write(cmd, (uint8_t *)buffer, length, 1);
	if (sendStop)
		i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(MODDEF_I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return (ESP_OK == ret) ? 0 : 1;
}

uint8_t modI2CActivate(modI2CConfiguration config)
{
	i2c_config_t conf;

#if defined(MODDEF_I2C_SDA_PIN) && defined(MODDEF_I2C_SCL_PIN)
	conf.sda_io_num = (-1 == config->sda) ? MODDEF_I2C_SDA_PIN : config->sda;
	conf.scl_io_num = (-1 == config->scl) ? MODDEF_I2C_SCL_PIN : config->scl;
#else
	if ((-1 == config->sda) || (-1 == config->scl)) {
		modLog("invalid sda/scl");
		return 1;
	}
	conf.sda_io_num = config->sda;
	conf.scl_io_num = config->scl;
#endif
	conf.master.clk_speed = config->hz ? config->hz : 100000;

	if ((conf.master.clk_speed == gHz) && (gSda == conf.sda_io_num) && (gScl == conf.scl_io_num))
		return 0;

	if (gHz) {
		i2c_driver_delete(MODDEF_I2C_PORT);
		gHz = 0;
	}

	conf.mode = I2C_MODE_MASTER;
#if MODDEF_I2C_PULLUPS
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
#else
	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
#endif
	conf.clk_flags = 0;
	if (ESP_OK != i2c_param_config(MODDEF_I2C_PORT, &conf)) {
		modLog("i2c_param_config fail");
		return 1;
	}

	if (ESP_OK != i2c_driver_install(MODDEF_I2C_PORT, conf.mode, 0, 0, 0)) {
		modLog("i2c_driver_install fail");
		return 1;
	}

	gHz = conf.master.clk_speed;
	gSda = conf.sda_io_num;
	gScl = conf.scl_io_num;

	if (config->timeout == 0) config->timeout = DEFAULT_ESP32_I2C_TIMEOUT;
	if (config->timeout != gTimeout){
		i2c_set_timeout(MODDEF_I2C_PORT, config->timeout);
		gTimeout = config->timeout;
	}

	return 0;
}
