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

#ifndef __MODGPIO_H__
#define __MODGPIO_H__

#include "em_gpio.h"

#include "stdint.h"

enum {
	kModGPIOInput 			= 0,
	kModGPIOInputPullUp		= 1,
	kModGPIOInputPullDown	= 2,
	kModGPIOInputPullUpDown	= 3,

	kModGPIOOutput 			= 8,
	kModGPIOOutputOpenDrain	= 9
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

extern int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode);
extern void modGPIOUninit(modGPIOConfiguration config);

extern int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode);

#define kModGPIOReadError (255)
extern uint8_t modGPIORead(modGPIOConfiguration config);
extern void modGPIOWrite(modGPIOConfiguration config, uint8_t value);

// callback on input value change...

#endif
