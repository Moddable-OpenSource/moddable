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

#ifndef __MODSERIAL_H__
#define __MODSERIAL_H__

#include "stdint.h"

typedef struct modSerialConfigurationRecord modSerialConfigurationRecord;
typedef struct modSerialConfigurationRecord *modSerialConfiguration;

struct modSerialConfigurationRecord {
	uint32_t					baudrate;
	int16_t						rx;
	int16_t						tx;
};

typedef struct modSerialConfigurationRecord modSerialConfigurationRecord;
typedef struct modSerialConfigurationRecord *modSerialConfiguration;

extern void modSerialInit(modSerialConfiguration config);		// required
extern void modSerialUninit(modSerialConfiguration config);	// optional

extern uint8_t modSerialRead(modSerialConfiguration config, uint8_t *buffer, uint16_t length);
extern uint8_t modSerialWrite(modSerialConfiguration config, const uint8_t *buffer);

#endif
