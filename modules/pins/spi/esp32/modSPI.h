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

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "stdint.h"

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

typedef void (*modSPIChipSelectCallback)(uint8_t active, modSPIConfiguration config);

struct modSPIConfigurationRecord {
	spi_device_handle_t			spi_dev;
	uint8_t						cs_pin;
	uint32_t					hz;
	modSPIChipSelectCallback	doChipSelect;
};

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

#define modSPIConfig(config, HZ, SPI_PORT, CS_PORT, CS_PIN, DOCHIPSELECT) \
	config.hz = HZ; \
	config.cs_pin = CS_PIN; \
	config.doChipSelect = DOCHIPSELECT; \
	if (0 != espStrCmp(SPI_PORT, "HSPI")) \
		xsUnknownError("invalid SPI port");

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
extern void modSPIFlush(void);
extern void modSPIActivateConfiguration(modSPIConfiguration config);

#endif
