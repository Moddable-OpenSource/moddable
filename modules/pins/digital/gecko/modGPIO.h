/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#ifndef __MODGPIO_H__
#define __MODGPIO_H__

#include "em_gpio.h"

#include "stdint.h"

enum {
	kModGPIOInput 			= 1,
	kModGPIOOutput 			= 2,

//	kModGPIOOutputOpenDrain,

	kModGPIOInputPullUp 	= 4,
//	kModGPIOInputPullDown
};

typedef struct modGPIOConfigurationRecord modGPIOConfigurationRecord;
typedef struct modGPIOConfigurationRecord *modGPIOConfiguration;

struct modGPIOConfigurationRecord {
	// contents vary by platform
	uint8_t		pin;
	uint8_t		portNum;
};

typedef struct modGPIOConfigurationRecord modGPIOConfigurationRecord;
typedef struct modGPIOConfigurationRecord *modGPIOConfiguration;

extern int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint8_t mode);
extern void modGPIOUninit(modGPIOConfiguration config);

extern int modGPIOSetMode(modGPIOConfiguration config, uint8_t mode);

extern uint8_t modGPIORead(modGPIOConfiguration config);						// 255 on failure
extern void modGPIOWrite(modGPIOConfiguration config, uint8_t value);

// callback on input value change...

#endif
