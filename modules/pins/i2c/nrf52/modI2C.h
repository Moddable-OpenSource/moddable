/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

#ifndef __MODI2C_H__
#define __MODI2C_H__

#include "nrf_twi_sensor.h"
#include "stdint.h"

typedef struct modI2CConfigurationRecord modI2CConfigurationRecord;
typedef struct modI2CConfigurationRecord *modI2CConfiguration;

struct modI2CConfigurationRecord {
	uint32_t					hz;
	uint32_t					timeout;
	nrf_drv_twi_frequency_t		nrfHz;
	int16_t						sda;
	int16_t						scl;
	uint8_t						address;		// 7-bit
};

#define modI2CConfig(config, HZ, SDA_PIN, SCL_PIN, ADDRESS, TIMEOUT)  \
    config.hz = HZ;                                                   \
    config.sda = SDA_PIN;                                             \
    config.scl = SCL_PIN;                                             \
    config.address = ADDRESS;										  \
	config.timeout = TIMEOUT;

typedef struct modI2CConfigurationRecord modI2CConfigurationRecord;
typedef struct modI2CConfigurationRecord *modI2CConfiguration;

extern void modI2CInit(modI2CConfiguration config);		// required
extern void modI2CUninit(modI2CConfiguration config);	// optional

extern uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop);
extern uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop);

#endif
