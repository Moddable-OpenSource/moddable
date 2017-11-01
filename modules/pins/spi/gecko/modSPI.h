/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#ifndef __MODSPI_H__
#define __MODSPI_H__

#include "em_usart.h"

#include "stdint.h"

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

#define modSPIConfig(config, HZ, SPI_PORT, CS_PORT, CS_PIN, DOCHIPSELECT) \
    config.hz = HZ; \
    config.doChipSelect = DOCHIPSELECT;

typedef void (*modSPIChipSelectCallback)(uint8_t active, modSPIConfiguration config);

struct modSPIConfigurationRecord {
	uint32_t					hz;
	uint32_t					clkdiv;		// stored speed
	modSPIChipSelectCallback	doChipSelect;
};

typedef struct modSPIConfigurationRecord modSPIConfigurationRecord;
typedef struct modSPIConfigurationRecord *modSPIConfiguration;

extern void modSPIInit(modSPIConfiguration config);
extern void modSPIUninit(modSPIConfiguration config);
extern void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray256To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPITxGray16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count);
extern void modSPIFlush(void);
extern void modSPIActivateConfiguration(modSPIConfiguration config);

#endif
