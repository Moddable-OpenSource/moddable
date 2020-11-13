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

#ifndef __MODSPI_H__
#define __MODSPI_H__

#include <xsPlatform.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "qapi_types.h"

#include "qurt_error.h"
#include "qurt_signal.h"
#include "qurt_thread.h"
#include "qapi_status.h"
#include "qapi_spi_master.h"
#include "qapi_tlmm.h"

#include "stdint.h"

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

typedef void (*modSPIChipSelectCallback)(uint8_t active, modSPIConfiguration config);

struct modSPIConfigurationRecord {
	void						*spi_dev;
	qapi_SPIM_Config_t			spi_config;
	qapi_SPIM_Transfer_t		tx;
	qapi_SPIM_Transfer_t		rx;
	qurt_signal_t				spi_signal;
	uint8_t						cs_pin;
	uint8_t						spiPort;
	uint8_t						sync;
	uint8_t						_pad;
	uint32_t					hz;
	modSPIChipSelectCallback	doChipSelect;
	uint8_t						mode;
};

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

#define modSPIConfig(config, HZ, SPI_PORT, CS_PORT, CS_PIN, DOCHIPSELECT) \
	config.hz = HZ; \
	config.cs_pin = ((uint8_t)(uint32_t)CS_PIN); \
	config.doChipSelect = DOCHIPSELECT; \
	config.spiPort = ((uint8_t)(uint32_t)SPI_PORT); \
	config.sync = 1; \
	config.mode = 0;

extern void modSPIInit(modSPIConfiguration config);
extern void modSPIUninit(modSPIConfiguration config);
extern void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxRGB565LEtoRGB444(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxRGB332To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray256To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPIFlush(void);
extern void modSPIActivateConfiguration(modSPIConfiguration config);
#define modSPISetSync(config, _sync) (config)->sync = (_sync)

#endif
