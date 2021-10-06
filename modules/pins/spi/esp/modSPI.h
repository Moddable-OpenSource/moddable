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

#ifndef __MODSPI_H__
#define __MODSPI_H__

#include "stdint.h"

#include "mc.defines.h"

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

typedef void (*modSPIChipSelectCallback)(uint8_t active, modSPIConfiguration config);

struct modSPIConfigurationRecord {
	uint32_t					hz;
	modSPIChipSelectCallback	doChipSelect;
	uint32_t					reserved;	// set during modSPIInit to SPI flash bits
	uint8_t						sync;
	uint8_t						mode;
	uint8_t						clock_pin;
	uint8_t						mosi_pin;
	uint8_t						miso_pin;
};

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

#define modSPIConfig(config, HZ, SPI_PORT, CS_PORT, CS_PIN, DOCHIPSELECT) \
	config.hz = HZ; \
	config.doChipSelect = DOCHIPSELECT; \
	config.sync = 1; \
	if (0 != espStrCmp(SPI_PORT, "HSPI")) \
		xsUnknownError("invalid SPI port"); \
	config.mode = 0; \
	config.clock_pin = MODDEF_SPI_SCK_PIN; \
	config.mosi_pin = MODDEF_SPI_MOSI_PIN; \
	config.miso_pin = MODDEF_SPI_MISO_PIN;

//	config.spiPort = SPI_PORT; \
//	config.csPort = CS_PORT; \
//	config.csPin = CS_PIN; \

extern void modSPIInit(modSPIConfiguration config);
extern void modSPIUninit(modSPIConfiguration config);
extern void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxRGB332To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray256To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxCLUT16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count, uint16_t *colors);
extern void modSPIFlush(void);
extern void modSPIActivateConfiguration(modSPIConfiguration config);
#define modSPISetSync(config, _sync) (config)->sync = (_sync)

#endif
