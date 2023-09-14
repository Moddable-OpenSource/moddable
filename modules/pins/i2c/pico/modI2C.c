/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

#include "hardware/i2c.h"

#if !defined(MODDEF_I2C_PULLUPS)
	#define MODDEF_I2C_PULLUPS	1
#endif

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static uint8_t modI2CActivate(modI2CConfiguration config);

static uint32_t gHz;		// non-zero when driver initialized
static uint16_t gSda;
static uint16_t gScl;
static i2c_inst_t *gInst;

static uint8_t checkValidI2C(uint32_t data, uint32_t clock, uint8_t *port)
{
    if ((data == 0 || data == 4 || data == 8 || data == 12 || data == 16 || data == 20 || data == 24 || data == 28) &&
         (clock == 1 || clock == 5 || clock == 9 || clock == 13 || clock == 17 || clock == 21 || clock == 25 || clock == 29)) {
            *port = 0;
            return 1;
    }
    if ((data == 2 || data == 6 || data == 10 || data == 14 || data == 18 || data == 22 || data == 26) &&
         (clock == 3 || clock == 7 || clock == 11 || clock == 15 || clock == 19 || clock == 23 || clock == 27)) {
            *port = 1;
            return 1;
    }
    return 0;
}

void modI2CInit(modI2CConfiguration config)
{
	uint8_t port;
#if defined(MODDEF_I2C_SDA_PIN) && defined(MODDEF_I2C_SCL_PIN)
	if (-1 == config->sda)
		config->sda = MODDEF_I2C_SDA_PIN;
	if (-1 == config->scl)
		config->scl = MODDEF_I2C_SCL_PIN;
#endif

	config->inst = NULL;

	if (checkValidI2C(config->sda, config->scl, &port)) {
		config->inst = port ? &i2c1_inst : &i2c0_inst;
		if (!config->hz)
			config->hz = 100000;
	}
}

void modI2CUninit(modI2CConfiguration config)
{
	modI2CActivate(NULL);
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	if (modI2CActivate(config))
		return 1;

	return (i2c_read_timeout_us(config->inst, config->address, buffer, length, !sendStop, config->timeout) < 0) ? 1 : 0;
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	if (modI2CActivate(config))
		return 1;

	return (i2c_write_timeout_us(config->inst, config->address, buffer, length, !sendStop, config->timeout) < 0) ? 1 : 0;
}

uint8_t modI2CActivate(modI2CConfiguration config)
{
	if (config) {
		if (NULL == config->inst) {
			modLog("invalid sda/scl");
			return 1;
		}

		if ((config->hz == gHz) && (config->sda == gSda) && (config->scl == gScl))
			return 0;
	}

	if (gInst) {
		i2c_deinit(gInst);
		gpio_set_function(gSda, GPIO_FUNC_NULL);
		gpio_set_function(gScl, GPIO_FUNC_NULL);
		gInst = NULL;
	}

	if (!config) {
		gHz = 0;
		return 0;
	}

	i2c_init(config->inst, config->hz);

	gpio_set_function(config->sda, GPIO_FUNC_I2C);
	gpio_set_function(config->scl, GPIO_FUNC_I2C);
#if MODDEF_I2C_PULLUPS
	gpio_pull_up(config->sda);
	gpio_pull_up(config->scl);
#endif

	gHz = config->hz;
	gSda = config->sda;
	gScl = config->scl;
	gInst = config->inst;

	return 0;
}
