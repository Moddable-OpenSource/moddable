/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#ifndef __MODI2C_H__
#define __MODI2C_H__

#include "em_gpio.h"
#include "em_i2c.h"
#include "stdint.h"
#include "mc.defines.h"

// #define I2C_PORT	gpioPortB
// #define I2C_SDA		11
// #define I2C_SCL		12

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

extern void modI2CInit(modI2CConfiguration config);		// required
extern void modI2CUninit(modI2CConfiguration config);	// optional

extern uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop);
extern uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop);

uint8_t modI2CWriteRead(modI2CConfiguration config, uint8_t *wBuffer, uint16_t wLength, uint8_t *rBuffer, uint16_t rLength);
uint8_t modI2CWriteWrite(modI2CConfiguration config, uint8_t *w1Buffer, uint16_t w1Length, uint8_t *w2Buffer, uint16_t w2Length);


#endif
