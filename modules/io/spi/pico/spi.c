/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support

#include "stddef.h"			// for offsetof macro

#include "modSPI.h"
#include "modGPIO.h"

#include "builtinCommon.h"

struct SPIRecord {
	modSPIConfigurationRecord	config;
	modGPIOConfigurationRecord	cs;
	xsSlot		obj;
	uint8_t		mosi;
	uint8_t		miso;
	uint8_t		clock;
	uint8_t		select;
	uint8_t		active;
	uint8_t		doUninit;
	uint8_t		transform;
};
typedef struct SPIRecord SPIRecord;
typedef struct SPIRecord *SPI;

#define kInvalidPin (0xff)

static void doChipSelect(uint8_t active, modSPIConfiguration config);
static void doChipSelectNOP(uint8_t active, modSPIConfiguration config);

static const uint8_t spi_miso_0[] = { 0, 4, 16, 20, 0xff };
static const uint8_t spi_mosi_0[] = { 3, 7, 19, 23, 0xff };
static const uint8_t spi_miso_1[] = { 8, 12, 24, 28, 0xff };
static const uint8_t spi_mosi_1[] = { 11, 15, 27, 0xff };

int spiPortForPin(uint8_t mosi, uint8_t miso)
{
	int i;
	if (kInvalidPin != mosi) {
		for (i=0; spi_mosi_0[i] != kInvalidPin; i++)
			if (spi_mosi_0[i] == mosi)
				return 0;
		for (i=0; spi_mosi_1[i] != kInvalidPin; i++)
			if (spi_mosi_1[i] == mosi)
				return 1;
	}
	if (kInvalidPin != miso) {
		for (i=0; spi_miso_0[i] != kInvalidPin; i++)
			if (spi_miso_0[i] == miso)
				return 0;
		for (i=0; spi_miso_1[i] != kInvalidPin; i++)
			if (spi_miso_1[i] == miso)
				return 1;
	}
}

//@@ pin values for in, out, and clock not being used or verified
void xs_spi_constructor(xsMachine *the)
{
	SPI spi;
	uint8_t mosi = kInvalidPin, miso = kInvalidPin, clock, select = kInvalidPin, active = 1, mode = 0;
	int hz, tmp;
	uint8_t spiPort;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	clock = builtinGetPin(the, &xsVar(0));
	if (!builtinIsPinFree(clock))
		xsUnknownError("in use");

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	if ((hz < 1) || (hz > 50000000))
		xsRangeError("invalid hz");

	if (xsmcHas(xsArg(0), xsID_out)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_out);
		mosi = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(mosi))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_in)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_in);
		miso = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(miso))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_select)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_select);
		select = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(select))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_active)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_active);
		active = xsmcToInteger(xsVar(0));
		if ((0 != active) && (1 != active))
			xsRangeError("invalid active");
	}

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		tmp = xsmcToInteger(xsVar(0));
		if (tmp & ~3)
			xsRangeError("invalid mode");
		mode = (uint8_t)tmp;
	}

	if ((kInvalidPin == mosi) && (kInvalidPin == miso))
		xsUnknownError("mosi or miso required");

//	xsmcGet(xsVar(0), xsArg(0), xsID_port);
//	tmp = xsmcToInteger(xsVar(0));
	tmp = spiPortForPin(mosi, miso);
	if (tmp < 0 || tmp > 2)
		xsRangeError("invalid port");
	spiPort = (uint8_t)tmp;

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	spi = c_calloc(1, sizeof(SPIRecord));
	if (!spi)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, spi);
	spi->obj = xsThis;
	xsRemember(spi->obj);
	spi->clock = clock;
	spi->mosi = mosi;
	spi->miso = miso;
	spi->select = select;
	spi->active = active;

	builtinUsePin(clock);
	if (kInvalidPin != mosi)
		builtinUsePin(mosi);
	if ((kInvalidPin != miso) && (mosi != miso))
		builtinUsePin(miso);
	if (kInvalidPin != select)
		builtinUsePin(select);

	modSPIConfig(spi->config, hz, spiPort, NULL, -1, (kInvalidPin == select) ? doChipSelectNOP : doChipSelect);

	spi->config.sync = true;
	spi->config.mode = mode;

	spi->config.clock_pin = clock;
	spi->config.mosi_pin = mosi; 
	spi->config.miso_pin = miso;

	modSPIInit(&spi->config);
	spi->doUninit = true;

	if (kInvalidPin != select)
		modGPIOInit(&spi->cs, NULL, select, kModGPIOOutput);
}

void xs_spi_destructor(void *data)
{
	SPI spi = data;
	if (!spi)
		return;

	if (spi->doUninit) {
		modSPIUninit(&spi->config);
		if (kInvalidPin != spi->select)
			modGPIOUninit(&spi->cs);
	}

	builtinFreePin(spi->clock);
	if (kInvalidPin != spi->mosi)
		builtinFreePin(spi->mosi);
	if ((kInvalidPin != spi->miso) && (spi->miso != spi->mosi))
		builtinFreePin(spi->miso);
	if (kInvalidPin != spi->select)
		builtinFreePin(spi->select);

	c_free(spi);
}

void xs_spi_close(xsMachine *the)
{
	SPI spi = xsmcGetHostData(xsThis);
	if (spi && xsmcGetHostDataValidate(xsThis, xs_spi_destructor)) {
		xsForget(spi->obj);
		xs_spi_destructor(spi);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_spi_read(xsMachine *the)
{
	SPI spi = xsmcGetHostDataValidate(xsThis, xs_spi_destructor);
	void *data;
	xsUnsignedValue count;

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsmcGetBufferWritable(xsArg(0), &data, &count);
		if (count > 65535)
			xsRangeError("unsupported byteLength");
	}
	else {
		int requested = xsmcToInteger(xsArg(0));
		if ((requested <= 0) || (requested > 65535))
			xsRangeError("unsupported byteLength");
		count = requested;
		data = xsmcSetArrayBuffer(xsResult, NULL, count);
	}

	modSPITxRx(&spi->config, data, (uint16_t)count);
}

void xs_spi_write(xsMachine *the)
{
	SPI spi = xsmcGetHostDataValidate(xsThis, xs_spi_destructor);
	void *data;
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(0), &data, &count);
	if (count > 65535)
		xsRangeError("unsupported byteLength");

	if (spi->transform)
		modSPITxSwap16(&spi->config, (uint8_t *)data, (uint16_t)count);
	else
		modSPITx(&spi->config, (uint8_t *)data, (uint16_t)count);
}

void xs_spi_transfer(xsMachine *the)
{
	SPI spi = xsmcGetHostDataValidate(xsThis, xs_spi_destructor);
	void *data;
	xsUnsignedValue count;

	xsmcGetBufferWritable(xsArg(0), &data, &count);
	if (count > 65535)
		xsRangeError("unsupported byteLength");

	modSPITxRx(&spi->config, (uint8_t *)data, (uint16_t)count);
}

void xs_spi_flush(xsMachine *the)
{
	SPI spi = xsmcGetHostDataValidate(xsThis, xs_spi_destructor);

	modSPIFlush();

	if (xsmcArgc && xsmcTest(xsArg(0)))
		modSPIActivateConfiguration(NULL);
}

void xs_spi_set_transform(xsMachine *the)
{
	SPI spi = xsmcGetHostDataValidate(xsThis, xs_spi_destructor);

	spi->transform = xsmcTest(xsArg(0));
}

void doChipSelect(uint8_t active, modSPIConfiguration config)
{
	SPI spi = (SPI)(((char *)config) - offsetof(SPIRecord, config));

	modGPIOWrite(&spi->cs, active ? spi->active : (1 - spi->active));
}

void doChipSelectNOP(uint8_t active, modSPIConfiguration config)
{
}
