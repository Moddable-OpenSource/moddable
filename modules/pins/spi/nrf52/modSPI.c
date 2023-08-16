/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
#include "xsmc.h"
#include "mc.xs.h"
#include "xsHost.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "modSPI.h"
#include "mc.defines.h"

#define IRAM_ATTR

#ifndef MODDEF_SPI_MISO_PIN
	#define MODDEF_SPI_MISO_PIN	NRF_GPIO_PIN_MAP(0,2)
#endif
#ifndef MODDEF_SPI_MOSI_PIN
	#define MODDEF_SPI_MOSI_PIN	NRF_GPIO_PIN_MAP(1,15)
#endif
#ifndef MODDEF_SPI_SCK_PIN
	#define MODDEF_SPI_SCK_PIN	NRF_GPIO_PIN_MAP(1,14)
#endif

#define SPI_CHUNKSIZE	(480 * 8)

#ifndef MODDEF_SPI_BUFFERSIZE
	#define MODDEF_SPI_BUFFERSIZE	SPI_CHUNKSIZE
#endif

#if MODDEF_SPI_BUFFERSIZE < 240
	#error "SPI buffer must be at least 240 bytes"
#endif

typedef uint16_t (*modSPIBufferLoader)(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);

static uint16_t modSpiLoadBufferAsIs(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);
static uint16_t modSpiLoadBufferSwap16(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);
static uint16_t modSpiLoadBufferRGB565LEtoRGB444(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);
static uint16_t modSpiLoadBufferRGB332To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);
static uint16_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);
static uint16_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut);

static modSPIConfiguration gConfig;
static uint8_t *gSPIData;
static volatile int16_t gSPIDataCount = -1;
static modSPIBufferLoader gSPIBufferLoader;
//static uint16_t *gCLUT16;		//@@ unused


uint32_t *gSPITxBuffer;
uint32_t *gSPIRxBuffer;

static uint8_t gSPIInited;

static const nrfx_spim_t gSPI = NRFX_SPIM_INSTANCE(MODDEF_SPI_INTERFACE);

static void spi_callback(nrfx_spim_evt_t const *event, void *refcon);

static int loadData(modSPIConfiguration config, uint8_t idx) {
	uint16_t loaded, bitsOut;

	if (gSPIDataCount <= 0)
		return 0;

	config->loadIdx = idx;
	loaded = (gSPIBufferLoader)(gSPIData, (gSPIDataCount <= MODDEF_SPI_BUFFERSIZE) ? gSPIDataCount : MODDEF_SPI_BUFFERSIZE, &bitsOut);
	gSPIDataCount -= loaded;
	gSPIData += loaded;
	config->transfer[idx].tx_length = loaded;
	config->transfer[idx].rx_length = loaded;

	config->loaded[idx] = 1;
	return (gSPIDataCount > 0);
}

static void startBulk(modSPIConfiguration config) {
	config->transIdx = 0;
	if (loadData(config, 0))
		loadData(config, 1);

	spi_callback(NULL, config);
}

void spi_callback(nrfx_spim_evt_t const *event, void *refcon)
{
	modSPIConfiguration config = (modSPIConfiguration)refcon;
	nrfx_err_t err;
	uint8_t alt = 0;

	CRITICAL_REGION_ENTER();

	if (event) {
		switch (event->type) {
			case NRFX_SPIM_EVENT_DONE:
				config->loaded[config->transIdx] = 0;
				config->transIdx = !config->transIdx;
				break;
			default:
				break;
		}
	}

	if (gSPIDataCount == 0 && !config->loaded[config->transIdx])
		gSPIDataCount = -1;

done:
	CRITICAL_REGION_EXIT();

	if (config->loaded[config->transIdx]) {
		// start transfer
		err = nrfx_spim_xfer(&gSPI, &config->transfer[config->transIdx], 0);
	}

	if (!config->loaded[!config->transIdx])
		loadData(config, !config->transIdx);

	return;
}


void modSPIInit(modSPIConfiguration config)
{
	int ret;

	if (!gSPIInited) {
		config->spi_config.sck_pin = (254 == config->clock_pin) ? MODDEF_SPI_SCK_PIN : ((255 == config->clock_pin) ? NRFX_SPIM_PIN_NOT_USED : config->clock_pin);
		config->spi_config.mosi_pin = (254 == config->mosi_pin) ? MODDEF_SPI_MOSI_PIN : ((255 == config->mosi_pin) ? NRFX_SPIM_PIN_NOT_USED : config->mosi_pin);
		config->spi_config.miso_pin = (254 == config->miso_pin) ?  MODDEF_SPI_MISO_PIN : ((255 == config->miso_pin) ? NRFX_SPIM_PIN_NOT_USED : config->miso_pin);
//		config->spi_config.ss_pin = config->cs_pin;
		config->spi_config.ss_pin = NRFX_SPIM_PIN_NOT_USED;
		config->spi_config.ss_active_high = false;
		config->spi_config.irq_priority = 7; // 7 - APP_IRQ_PRIORITY_LOWEST;
		config->spi_config.orc = 0xFF;
		config->spi_config.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST;

//config->mode = 3;
		switch (config->mode) {
			case 3:
				config->spi_config.mode = NRF_SPIM_MODE_3;	// CPOL = 1, CPHA = 1
				break;
			case 2:
				config->spi_config.mode = NRF_SPIM_MODE_2;	// CPOL = 1, CPHA = 0
				break;
			case 1:
				config->spi_config.mode = NRF_SPIM_MODE_1;	// CPOL = 0, CPHA = 1
				break;
			case 0:
			default:
				config->spi_config.mode = NRF_SPIM_MODE_0;	// CPOL = 0, CPHA = 0
		}

		if (config->hz > 16000000)
			config->spi_config.frequency = NRF_SPIM_FREQ_32M;
		else if (config->hz > 8000000)
			config->spi_config.frequency = NRF_SPIM_FREQ_16M;
		else if (config->hz > 4000000)
			config->spi_config.frequency = NRF_SPIM_FREQ_8M;
		else if (config->hz > 2000000)
			config->spi_config.frequency = NRF_SPIM_FREQ_4M;
		else if (config->hz > 1000000)
			config->spi_config.frequency = NRF_SPIM_FREQ_2M;
		else if (config->hz > 500000)
			config->spi_config.frequency = NRF_SPIM_FREQ_1M;
		else if (config->hz > 250000)
			config->spi_config.frequency = NRF_SPIM_FREQ_500K;
		else if (config->hz > 125000)
			config->spi_config.frequency = NRF_SPIM_FREQ_250K;
		else
			config->spi_config.frequency = NRF_SPIM_FREQ_125K;
			
		ret = nrfx_spim_init(&gSPI, &config->spi_config, spi_callback, config);
		if (ret) {
			xsBeginHost(config->the);
			xsTraceLeft("spi_init - status non zero", "spi");
			xsEndHost(config->the);
		}

#ifdef MODDEF_SPI_HIGH_POWER
		// set IO to high drive
		nrf_gpio_cfg(
			MODDEF_SPI_MOSI_PIN,
			NRF_GPIO_PIN_DIR_OUTPUT,
			NRF_GPIO_PIN_INPUT_DISCONNECT,
			NRF_GPIO_PIN_NOPULL,
			NRF_GPIO_PIN_H0H1,
			NRF_GPIO_PIN_NOSENSE);

		nrf_gpio_cfg(
			MODDEF_SPI_SCK_PIN,
			NRF_GPIO_PIN_DIR_OUTPUT,
			NRF_GPIO_PIN_INPUT_DISCONNECT,
			NRF_GPIO_PIN_NOPULL,
			NRF_GPIO_PIN_H0H1,
			NRF_GPIO_PIN_NOSENSE);
#endif

		// trans and rcv organized as 2xTX buffer and 1xRX buffer
		gSPITxBuffer = (uint32_t *)c_malloc(MODDEF_SPI_BUFFERSIZE * 3);
		gSPIRxBuffer = (uint32_t *)((MODDEF_SPI_BUFFERSIZE * 2) + (uintptr_t)gSPITxBuffer);
		config->transfer[0].p_tx_buffer = (uint8_t*)gSPITxBuffer;
		config->transfer[0].tx_length = 0;
		config->transfer[0].p_rx_buffer = (uint8_t*)gSPIRxBuffer;
		config->transfer[0].rx_length = 0;
		config->transfer[1].p_tx_buffer = (uint8_t*)gSPITxBuffer + MODDEF_SPI_BUFFERSIZE;
		config->transfer[1].tx_length = 0;
		config->transfer[1].p_rx_buffer = (uint8_t*)gSPIRxBuffer;
		config->transfer[1].rx_length = 0;

		gSPIInited = true;
	}

	if (NULL == gConfig) {
		gSPIData = NULL;
		gSPIDataCount = -1;

		gConfig = config;
	}
	else
		modSPIFlush();

	(config->doChipSelect)(0, config);
}

void modSPIUninit(modSPIConfiguration config)
{
	if (config == gConfig)
		modSPIActivateConfiguration(NULL);

//@@ should only be done on last SPI client closing
	nrfx_spim_uninit(&gSPI);
	gSPIInited = false;
}

void modSPIActivateConfiguration(modSPIConfiguration config)
{
	int ret;

	modSPIFlush();

	if (config == gConfig)
		return;

	if (gConfig) {
		(gConfig->doChipSelect)(0, gConfig);
	}

	gConfig = config;
	if (gConfig) {
		(gConfig->doChipSelect)(1, gConfig);
	}
}

// data must be long aligned (doesn't matter on qca4020 or gecko)
uint16_t IRAM_ATTR modSpiLoadBufferAsIs(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint32_t *from = (uint32_t *)data, *to = (uint32_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	uint8_t longs;
	uint16_t result = bytes;

	*bitsOut = bytes << 3;

	while (bytes >= 64) {
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;
		bytes -= 64;
	}

	longs = (bytes + 3) >> 2;
	while (longs--)
		*to++ = *from++;

	return result;
}

uint16_t IRAM_ATTR modSpiLoadBufferSwap16(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint32_t *from = (uint32_t *)data;
	uint32_t *to = (uint32_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	const uint32_t mask = 0x00ff00ff;
	uint32_t twoPixels;
	uint16_t result = bytes;
	uint8_t longs;

	*bitsOut = bytes << 3;

	while (bytes >= 64) {
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		bytes -= 64;
	}

	longs = (bytes + 3) >> 2;
	while (longs--) {
		twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
	}

	return result;

}

uint16_t IRAM_ATTR modSpiLoadBufferRGB565LEtoRGB444(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint16_t *from = (uint16_t *)data;
	uint8_t *to = (uint8_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	uint16_t remain;

	if (bytes > MODDEF_SPI_BUFFERSIZE)
		bytes = MODDEF_SPI_BUFFERSIZE;

	*bitsOut = (bytes >> 1) * 12;

	remain = bytes >> 2;
	while (remain--) {
		uint8_t r, g, b;
		uint16_t rgb565 = *from++;
		r = rgb565 >> 12, g = (rgb565 >> 7) & 0x0F, b = (rgb565 >> 1) & 0x0F;
		*to++ = (r << 4) | g;

		rgb565 = *from++;
		r = rgb565 >> 12;
		*to++ = (b << 4) | r;
		g = (rgb565 >> 7) & 0x0F, b = (rgb565 >> 1) & 0x0F;
		*to++ = (g << 4) | b;
	}

	if (bytes & 2) {
		uint8_t r, g, b;
		uint16_t rgb565 = *from;
		r = rgb565 >> 12, g = (rgb565 >> 7) & 0x0F, b = (rgb565 >> 1) & 0x0F;
		*to++ = (r << 4) | g;
		*to = b << 4;
	}

	return bytes;		// input bytes consumed
}

uint16_t IRAM_ATTR modSpiLoadBufferRGB332To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	uint16_t remain;

	if (bytes > (MODDEF_SPI_BUFFERSIZE >> 1))
		bytes = MODDEF_SPI_BUFFERSIZE >> 1;

	*bitsOut = bytes << 4;		// output bits are double input bits

	remain = bytes;
	while (remain >= 2) {
		uint8_t rgb332, r, g, b;
		uint16_t pixela, pixelb;

		rgb332 = *from++;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixela = (r << 11) | (g << 5) | b;
		pixela = (pixela >> 8) | (pixela << 8);

		rgb332 = *from++;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixelb = (r << 11) | (g << 5) | b;
		pixelb = (pixelb >> 8) | (pixelb << 8);

		*to++ = (pixelb << 16) | pixela;
		remain -= 2;
	}

	if (remain) {
		uint16_t pixela;
		uint8_t rgb332 = *from++, r, g, b;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixela = (r << 11) | (g << 5) | b;
		pixela = (pixela >> 8) | (pixela << 8);
		*to++ = (0 << 16) | pixela;
	}

	return bytes;		// input bytes consumed
}

uint16_t IRAM_ATTR modSpiLoadBufferGray256To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	uint16_t remain;

	if (bytes > (MODDEF_SPI_BUFFERSIZE >> 1))
		bytes = MODDEF_SPI_BUFFERSIZE >> 1;

	*bitsOut = bytes << 4;		// output bits are double input bits

	remain = bytes;
	while (remain >= 2) {
		uint8_t gray;
		uint16_t pixela, pixelb;

		gray = *from++;
		pixela = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);
		pixela = (pixela >> 8) | (pixela << 8);

		gray = *from++;
		pixelb = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);
		pixelb = (pixelb >> 8) | (pixelb << 8);

		*to++ = (pixelb << 16) | pixela;
		remain -= 2;
	}

	if (remain) {
		uint8_t gray = *from++;
		uint16_t pixela = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);
		pixela = (pixela >> 8) | (pixela << 8);
		*to++ = (0 << 16) | pixela;
	}

	return bytes;		// input bytes consumed
}

uint16_t IRAM_ATTR modSpiLoadBufferGray16To16BE(uint8_t *data, uint16_t bytes, uint16_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)(gSPITxBuffer + (gConfig->loadIdx * SPI_CHUNKSIZE));
	uint16_t i;

	if (bytes > (MODDEF_SPI_BUFFERSIZE >> 2))
		bytes = MODDEF_SPI_BUFFERSIZE >> 2;
	*bitsOut = bytes << 5;		// output bits are quadruple input bits

	for (i = 0; i < bytes; i++) {
		uint8_t gray;
		uint8_t twoPixels = *from++;
		uint16_t pixela, pixelb;

		gray = (twoPixels >> 2) & 0x3C;
		gray |= gray >> 4;				// 6 bits of gray
		pixela = gray >> 1;				// 5 bita of gray
		pixela |= (pixela << 11) | (gray << 5);
		pixela = (pixela >> 8) | (pixela << 8);

		gray = (twoPixels << 2) & 0x3C;
		gray |= gray >> 4;
		pixelb = gray >> 1;
		pixelb |= (pixelb << 11) | (gray << 5);
		pixelb = (pixelb >> 8) | (pixelb << 8);

		*to++ = (pixelb << 16) | pixela;
	}

	return bytes;		// input bytes consumed
}

// N.B. callers assume this function is synchronous
void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	int ret;

	modSPIActivateConfiguration(config);

	if (count > 255) {		// need to loop
		modLog("TxRx over 255 bytes unimplemented");
		return;
	}

	gSPIDataCount = 0;
	config->transIdx = 0;

	c_memcpy(gSPITxBuffer, data, count);
	config->transfer[0].tx_length = count;
	config->transfer[0].rx_length = count;

	ret = nrfx_spim_xfer(&gSPI, &config->transfer[0], 0);
	if (ret) {
		xsBeginHost(config->the);
		xsTraceLeft("spiTxRx - status non zero", "spi");
		xsEndHost(config->the);
	}
// wait for completion
	while (gSPIDataCount != -1)
        taskYIELD();

	c_memcpy(data, gSPIRxBuffer, count);
}

void modSPIFlush(void)
{
	while (gSPIDataCount != -1)
        taskYIELD();
}

static void modSPITxCommon(modSPIConfiguration config, uint8_t *data, uint16_t count, modSPIBufferLoader loader)
{
	modSPIActivateConfiguration(config);

	gSPIBufferLoader = loader;
	gSPIData = data;
	gSPIDataCount = count;

	startBulk(config);

	if (config->sync)
		modSPIFlush();
}

void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferAsIs);
}

void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferSwap16);
}

void modSPITxRGB565LEtoRGB444(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferRGB565LEtoRGB444);
}

void modSPITxRGB332To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferRGB332To16BE);
}

void modSPITxGray256To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferGray256To16BE);
}

void modSPITxGray16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferGray16To16BE);
}

void modSPITxCLUT16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count, uint16_t *colors)
{
//	gCLUT16 = colors;
//	modSPITxCommon(config, data, count, modSpiLoadBufferGray16To16BE);
//	xsUnknownError("need to implement CLUT16to16BE\n");
}

