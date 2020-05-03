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

#ifndef __MODI2C_H__
#define __MODI2C_H__

#include "em_gpio.h"
#include "em_i2c.h"
#include "stdint.h"
#include "mc.defines.h"

#if !defined(MODDEF_I2C_INTERFACE_I2C)
    #define MODDEF_I2C_INTERFACE_I2C 0
#endif

#if MODDEF_I2C_INTERFACE_I2C == 0
    #define I2C_PORT    I2C0
    #define I2C_CLOCK   cmuClock_I2C0
    #define I2C_IRQ     I2C0_IRQn
#elif MODDEF_I2C_INTERFACE_I2C == 1
    #define I2C_PORT    I2C1
    #define I2C_CLOCK   cmuClock_I2C1
    #define I2C_IRQ     I2C1_IRQn
#elif MODDEF_I2C_INTERFACE_I2C == 2
    #define I2C_PORT    I2C2
    #define I2C_CLOCK   cmuClock_I2C2
    #define I2C_IRQ     I2C2_IRQn
#else
    #error bad I2C port
#endif

#ifndef MODDEF_I2C_SDA_LOCATION
    #define MODDEF_I2C_SDA_LOCATION MODDEF_I2C_LOCATION
#endif
#ifndef MODDEF_I2C_SCL_LOCATION 
    #define MODDEF_I2C_SCL_LOCATION MODDEF_I2C_LOCATION
#endif

typedef struct modI2CConfigurationRecord modI2CConfigurationRecord;
typedef struct modI2CConfigurationRecord *modI2CConfiguration;

struct modI2CConfigurationRecord {
	uint32_t				hz;
	int16_t					sda;
	int16_t					scl;
	uint8_t					address;	// 7-bit shifted up one bit for gecko
	I2C_TransferSeq_TypeDef	tcb;
};

typedef struct modI2CConfigurationRecord modI2CConfigurationRecord;
typedef struct modI2CConfigurationRecord *modI2CConfiguration;

#define modI2CConfig(config, HZ, SDA_PIN, SCL_PIN, ADDRESS, TIMEOUT)  \
    config.hz = HZ;                                                   \
    config.sda = SDA_PIN;                                             \
    config.scl = SCL_PIN;                                             \
    config.address = ADDRESS;

extern void modI2CInit(modI2CConfiguration config);		// required
extern void modI2CUninit(modI2CConfiguration config);	// optional

I2C_TransferReturn_TypeDef modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop);
I2C_TransferReturn_TypeDef modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop);

I2C_TransferReturn_TypeDef modI2CWriteRead(modI2CConfiguration config, uint8_t *wBuffer, uint16_t wLength, uint8_t *rBuffer, uint16_t rLength);
I2C_TransferReturn_TypeDef modI2CWriteWrite(modI2CConfiguration config, uint8_t *w1Buffer, uint16_t w1Length, uint8_t *w2Buffer, uint16_t w2Length);


#endif
