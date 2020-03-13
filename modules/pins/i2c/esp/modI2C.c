/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "twi.h"		// i2c

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static void modI2CActivate(modI2CConfiguration config);

static modI2CConfigurationRecord gI2CConfig;

static uint8_t gI2CClientCount = 0;

void modI2CInit(modI2CConfiguration config)
{
	gI2CConfig.hz = 1;		// garbage value to ensure mismatch

	gI2CClientCount += 1;
}

void modI2CUninit(modI2CConfiguration config)
{
	gI2CClientCount -= 1;
	if (0 == gI2CClientCount)
		twi_stop();
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	modI2CActivate(config);
	return twi_readFrom(config->address, buffer, length, sendStop);
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	modI2CActivate(config);
	return twi_writeTo(config->address, (void *)buffer, length, sendStop);
}

void modI2CActivate(modI2CConfiguration config)
{
	if ((gI2CConfig.sda == config->sda) && (gI2CConfig.scl == config->scl) && (gI2CConfig.hz == config->hz) && gI2CConfig.timeout == config->timeout)
		return;

	gI2CConfig = *config;

#if defined(MODDEF_I2C_SDA_PIN) && defined(MODDEF_I2C_SCL_PIN)
	twi_init((-1 == config->sda) ? MODDEF_I2C_SDA_PIN : config->sda, (-1 == config->scl) ? MODDEF_I2C_SCL_PIN : config->scl);
#else
	if ((-1 == config->sda) || (-1 == config->scl)) {
		modLog("invalid sda/scl");
		return;
	}
	twi_init(config->sda, config->scl);
#endif
	if (config->hz)
		twi_setClock(config->hz);
	if (config->timeout)
		twi_setClockStretchLimit(config->timeout);
}
