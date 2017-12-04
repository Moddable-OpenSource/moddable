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

#include "modI2C.h"

#include "driver/i2c.h"

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static void modI2CActivate(modI2CConfiguration config);

static modI2CConfigurationRecord gI2CConfig;

void modI2CInit(modI2CConfiguration config)
{
	i2c_config_t conf;

	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = config->sda;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = config->scl;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = config->hz ? config->hz : 100000;
	i2c_param_config(I2C_NUM_1, &conf);
	i2c_driver_install(I2C_NUM_1, conf.mode, 0, 0, 0);
}

void modI2CUninit(modI2CConfiguration config)
{
	i2c_driver_delete(I2C_NUM_1);
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;
	i2c_cmd_handle_t cmd;

	modI2CActivate(config);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (config->address << 1) | I2C_MASTER_READ, 1);
	if (length > 1)
		i2c_master_read(cmd, buffer, length - 1, 0);
	i2c_master_read(cmd, buffer + length - 1, 1, 1);
	if (sendStop)
		i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return (ESP_OK == ret) ? 0 : 1;
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;
	i2c_cmd_handle_t cmd;

	modI2CActivate(config);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (config->address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write(cmd, (uint8_t *)buffer, length, 1);
	if (sendStop)
		i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return (ESP_OK == ret) ? 0 : 1;
}

void modI2CActivate(modI2CConfiguration config)
{
	gI2CConfig = *config;
}
