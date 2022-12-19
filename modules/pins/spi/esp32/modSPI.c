/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

#include "xsHost.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "modSPI.h"

#include "mc.defines.h"

#include "esp_attr.h"		// IRAM_ATTR
#include "esp_heap_caps.h"	// MALLOC_CAP_DMA, heap_caps_malloc

#include "semphr.h"

#ifndef MODDEF_SPI_MISO_PIN
	#define MODDEF_SPI_MISO_PIN	12
#endif
#ifndef MODDEF_SPI_MOSI_PIN
	#define MODDEF_SPI_MOSI_PIN	13
#endif
#ifndef MODDEF_SPI_SCK_PIN
	#define MODDEF_SPI_SCK_PIN	14
#endif
#ifndef MODDEF_SPI_ESP32_TRANSACTIONS
	#define MODDEF_SPI_ESP32_TRANSACTIONS (3)
#endif
#ifndef MODDEF_SPI_ESP32_TRANSACTIONSIZE
	#define MODDEF_SPI_ESP32_TRANSACTIONSIZE (1024)
#endif

typedef uint32_t (*modSPIBufferLoader)(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);

static uint32_t modSpiLoadBufferAsIs(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferSwap16(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferRGB565LEtoRGB444(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferRGB332To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);
static uint32_t modSpiLoadBufferCLUT16To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut);

static modSPIConfiguration gConfig;
static uint8_t *gSPIData;
static volatile int32_t gSPIDataCount = -1;
static modSPIBufferLoader gSPIBufferLoader;
static uint16_t *gCLUT16;

#define kTransactions MODDEF_SPI_ESP32_TRANSACTIONS
static spi_transaction_t gTransaction[kTransactions];
static uint8_t gTransactionIndex = 0;
static volatile uint8_t gTransactionsPending = 0;

#define SPI_BUFFER_SIZE MODDEF_SPI_ESP32_TRANSACTIONSIZE

uint32_t *gSPITransactionBuffer[kTransactions];

static TaskHandle_t gSPITask;
static SemaphoreHandle_t gSPIMutex;

static void IRAM_ATTR queueTransfer(modSPIConfiguration config)
{
    uint32_t bitsOut;
    uint16_t loaded;
    spi_transaction_t *trans = &gTransaction[gTransactionIndex];

    loaded = (gSPIBufferLoader)(gSPIData, (gSPIDataCount <= SPI_BUFFER_SIZE) ? gSPIDataCount : SPI_BUFFER_SIZE, &bitsOut);
    gSPIDataCount -= loaded;
    gSPIData += loaded;

    trans->flags = 0;
    trans->cmd = 0;
    trans->addr = 0;
    trans->length = bitsOut;
    trans->rxlength = 0;
    trans->user = 0;
    trans->tx_buffer = gSPITransactionBuffer[gTransactionIndex];
    trans->rx_buffer = NULL;

	gTransactionsPending += 1;
    gTransactionIndex += 1;
    if (gTransactionIndex >= kTransactions)
        gTransactionIndex = 0;

	spi_device_queue_trans(config->spi_dev, trans, portMAX_DELAY);
}

void spiLoop(void *pvParameter)
{
	while (true) {
		uint32_t newState;
		spi_transaction_t *ret_trans;

		xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);
		xSemaphoreTake(gSPIMutex, portMAX_DELAY);

		if (!gConfig) {
			xSemaphoreGive(gSPIMutex);
			continue;
		}

		while (gTransactionsPending && (ESP_OK == spi_device_get_trans_result(gConfig->spi_dev, &ret_trans, 0)))
			gTransactionsPending -= 1;

		while ((gSPIDataCount > 0) && (gTransactionsPending < kTransactions))
			queueTransfer(gConfig);

		if (gSPIDataCount <= 0) {
			gSPIDataCount = -1;
			gSPIData = NULL;
		}

		xSemaphoreGive(gSPIMutex);
	}
}

static void IRAM_ATTR postTransfer(spi_transaction_t *transIn)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	int i = transIn - gTransaction;
	if ((i < 0) || (i >= kTransactions))
		return;

//	if (transIn->rx_buffer)
//		return;

	xTaskNotifyFromISR(gSPITask, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);

	if (xHigherPriorityTaskWoken)
		portYIELD_FROM_ISR();
}

static uint8_t gSPIInited;

void modSPIInit(modSPIConfiguration config)
{
	esp_err_t ret;

	if (!gSPIInited) {
		spi_bus_config_t buscfg;
        uint8_t i;

        gSPITransactionBuffer[0] = heap_caps_malloc(SPI_BUFFER_SIZE * kTransactions, MALLOC_CAP_DMA);      // use DMA capable memory for SPI driver
        for (i = 1; i < kTransactions; i++)
            gSPITransactionBuffer[i] = (uint32_t *)(SPI_BUFFER_SIZE + (uint8_t *)gSPITransactionBuffer[i - 1]);
		memset(&buscfg, 0, sizeof(buscfg));
		buscfg.miso_io_num = (254 == config->miso_pin) ? MODDEF_SPI_MISO_PIN : ((255 == config->miso_pin) ? -1 : config->miso_pin);
		buscfg.mosi_io_num = (254 == config->mosi_pin) ? MODDEF_SPI_MOSI_PIN : ((255 == config->mosi_pin) ? -1 : config->mosi_pin);
		buscfg.sclk_io_num = (254 == config->clock_pin) ? MODDEF_SPI_SCK_PIN : ((255 == config->clock_pin) ? -1 : config->clock_pin);
		buscfg.quadwp_io_num = -1;
		buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = MODDEF_SPI_ESP32_TRANSACTIONSIZE;

	#if kCPUESP32S3 || kCPUESP32C3
		ret = spi_bus_initialize(config->spiPort, &buscfg, SPI_DMA_CH_AUTO);
	#elif kCPUESP32S2
		ret = spi_bus_initialize(config->spiPort, &buscfg, config->spiPort);
	#else
		ret = spi_bus_initialize(config->spiPort, &buscfg, 1);
	#endif
		if (ret) {
            free(gSPITransactionBuffer[0]);
            gSPITransactionBuffer[0] = NULL;
			return;
		}

		gSPIData = NULL;
		gSPIDataCount = -1;

		gSPIMutex = xSemaphoreCreateMutex();
		xTaskCreate(spiLoop, "spiLoop", 2048 + XT_STACK_EXTRA_CLIB, NULL, 10, &gSPITask);
	}

    spi_device_interface_config_t devcfg;
	memset(&devcfg, 0, sizeof(devcfg));
	devcfg.clock_speed_hz = config->hz;
	devcfg.mode = config->mode & 3;
	devcfg.spics_io_num = config->cs_pin;		// set to -1 if none
	devcfg.queue_size = kTransactions;
	devcfg.pre_cb = NULL;
	devcfg.post_cb = postTransfer;
	devcfg.input_delay_ns = config->miso_delay;

	ret = spi_bus_add_device(config->spiPort, &devcfg, &config->spi_dev);
	if (ret) {
        free(gSPITransactionBuffer[0]);
        gSPITransactionBuffer[0] = NULL;
		spi_bus_free(config->spiPort);
		return;
	}

	gSPIInited += 1;
}

void modSPIUninit(modSPIConfiguration config)
{
	if (config == gConfig)
		modSPIActivateConfiguration(NULL);

	if (config->spi_dev) {
		spi_bus_remove_device(config->spi_dev);
		config->spi_dev = NULL;
	}

	gSPIInited -= 1;
	if (gSPIInited)
		return;

	spi_bus_free(config->spiPort);

    free(gSPITransactionBuffer[0]);
    gSPITransactionBuffer[0] = NULL;
	vTaskDelete(gSPITask);
	gSPITask = NULL;
	vSemaphoreDelete(gSPIMutex);
	gSPIMutex = NULL;
}

void modSPIActivateConfiguration(modSPIConfiguration config)
{
	modSPIFlush();

	if (config == gConfig)
		return;

	if (gConfig)
		(gConfig->doChipSelect)(0, gConfig);

	gConfig = config;
	if (gConfig)
		(gConfig->doChipSelect)(1, gConfig);
}

// data must be long aligned
uint32_t IRAM_ATTR modSpiLoadBufferAsIs(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint32_t *from = (uint32_t *)data, *to = gSPITransactionBuffer[gTransactionIndex];
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

uint32_t IRAM_ATTR modSpiLoadBufferSwap16(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint32_t *from = (uint32_t *)data;
	uint32_t *to = gSPITransactionBuffer[gTransactionIndex];
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

uint32_t IRAM_ATTR modSpiLoadBufferRGB565LEtoRGB444(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint16_t *from = (uint16_t *)data;
	uint8_t *to = (uint8_t *)gSPITransactionBuffer[gTransactionIndex];
	uint16_t remain;

	if (bytes > SPI_BUFFER_SIZE)
		bytes = SPI_BUFFER_SIZE;

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

uint32_t IRAM_ATTR modSpiLoadBufferRGB332To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer[gTransactionIndex];
	uint16_t remain;

	if (bytes > (SPI_BUFFER_SIZE >> 1))
		bytes = SPI_BUFFER_SIZE >> 1;

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

uint32_t IRAM_ATTR modSpiLoadBufferGray256To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer[gTransactionIndex];
	uint16_t remain;

	if (bytes > (SPI_BUFFER_SIZE >> 1))
		bytes = SPI_BUFFER_SIZE >> 1;

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

uint32_t IRAM_ATTR modSpiLoadBufferGray16To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer[gTransactionIndex];
	uint16_t i;

	if (bytes > (SPI_BUFFER_SIZE >> 2))
		bytes = SPI_BUFFER_SIZE >> 2;
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

uint32_t IRAM_ATTR modSpiLoadBufferCLUT16To16BE(uint8_t *data, uint32_t bytes, uint32_t *bitsOut)
{
	uint16_t *clut16_t = gCLUT16;
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer[gTransactionIndex];
	uint16_t i;

	if (bytes > (SPI_BUFFER_SIZE >> 2))
		bytes = SPI_BUFFER_SIZE >> 2;
	*bitsOut = bytes << 5;		// output bits are quadruple input bits

	for (i = 0; i < bytes; i++) {
		uint16_t pixelA, pixelB;
		uint8_t twoPixels = *from++;

		pixelA = clut16_t[twoPixels >> 4];
		pixelA = (pixelA >> 8) | (pixelA << 8);

		pixelB = clut16_t[twoPixels & 0x0F];
		pixelB = (pixelB >> 8) | (pixelB << 8);
		*to++ = (pixelB << 16) | pixelA;
	}

	return bytes;		// input bytes consumed
}

// N.B. callers assume this funtion is synchronous
void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	esp_err_t ret;
	spi_transaction_t t;

	modSPIActivateConfiguration(config);

	memset(&t, 0, sizeof(t));
	t.length = 8 * count;
	t.tx_buffer = data;

	t.rxlength = t.length;
	t.rx_buffer = data;
	ret = spi_device_transmit(config->spi_dev, &t);

	if (0 > ret)
		modLog("problems sending spi message");
}

void modSPIFlush(void)
{
	while (true) {
		xSemaphoreTake(gSPIMutex, portMAX_DELAY);
		int active = gTransactionsPending || (gSPIDataCount > 0);
		xSemaphoreGive(gSPIMutex);
		if (!active) break;
        taskYIELD();
	}
}

static void modSPITxCommon(modSPIConfiguration config, uint8_t *data, uint16_t count, modSPIBufferLoader loader)
{
	if (!config->sync && (config == gConfig)) {
		while (true) {
			xSemaphoreTake(gSPIMutex, portMAX_DELAY);
			int active = (kTransactions == gTransactionsPending) || (gSPIDataCount > 0);
			if (!active) break;
			xSemaphoreGive(gSPIMutex);
			taskYIELD();
		}
	}
	else {
		modSPIActivateConfiguration(config);
		xSemaphoreTake(gSPIMutex, portMAX_DELAY);
	}

	gSPIBufferLoader = loader;
    gSPIData = data;
    gSPIDataCount = count;

    if (config->sync) {
        queueTransfer(config);
		xSemaphoreGive(gSPIMutex);
		modSPIFlush();
    }
	else {
		xSemaphoreGive(gSPIMutex);
		xTaskNotify(gSPITask, (uintptr_t)config, eSetValueWithOverwrite);
	}
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
	gCLUT16 = colors;
	modSPITxCommon(config, data, count, modSpiLoadBufferCLUT16To16BE);
}
