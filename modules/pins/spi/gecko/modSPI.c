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

#include "xsHost.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "modSPI.h"
#include "mc.defines.h"

	#include "em_device.h"
	#include "em_cmu.h"
	#include "em_emu.h"
	#include "em_gpio.h"
	#include "em_usart.h"
	#include "em_system.h"

#if !defined(MODDEF_SPI_INTERFACE_USART)
	#error "need a USART interface for spi"
#endif

#if MODDEF_SPI_INTERFACE_USART == 0
	#define SPI_USART	USART0
	#define SPI_CLOCK	cmuClock_USART0
	#define SPI_TX_IRQ	USART0_TX_IRQn
#elif MODDEF_SPI_INTERFACE_USART == 1
	#define SPI_USART	USART1
	#define SPI_CLOCK	cmuClock_USART1
	#define SPI_TX_IRQ	USART1_TX_IRQn
#elif MODDEF_SPI_INTERFACE_USART == 2
	#define SPI_USART	USART2
	#define SPI_CLOCK	cmuClock_USART2
	#define SPI_TX_IRQ	USART2_TX_IRQn
#else
	#error bad SPI port
#endif

#if !defined(MODDEF_SPI_MOSI_LOCATION)
	#define MODDEF_SPI_MOSI_LOCATION MODDEF_SPI_LOCATION
#endif
#if !defined(MODDEF_SPI_MISO_LOCATION)
	#define MODDEF_SPI_MISO_LOCATION MODDEF_SPI_LOCATION
#endif
#if !defined(MODDEF_SPI_SCK_LOCATION)
	#define MODDEF_SPI_SCK_LOCATION MODDEF_SPI_LOCATION
#endif

typedef uint16_t (*modSPIBufferLoader)(uint8_t *data, uint16_t bytes);

static uint16_t modSpiLoadBufferAsIs(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferSwap16(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint16_t bytes);

static modSPIConfiguration gConfig;
//static uint16_t *gCLUT16;		//@@ unused

// SPI_BUFFER_SIZE can be larger than 64... tested up to 512
//#define SPI_BUFFER_SIZE (64)
#define SPI_BUFFER_SIZE (256)
static uint32_t gSPITransactionBuffer[SPI_BUFFER_SIZE / sizeof(uint32_t)];

static uint8_t gSPIInited;

USART_InitSync_TypeDef spiInit;
#if 0
 = {
	usartEnable,	/* Enable RX/TX when init completed. */
	0, // 48000000,		/* 48MHz clock (0 for current configd reference clock). */
	48000000,		/* was 1000000,           1 Mbits/s. */
	usartDatabits8, /* 8 databits. */
	true,			/* Master mode. */
	true,			/* Send most significant bit first. */
	usartClockMode0,   /* Clock idle low, sample on rising edge. */
	false,             /* Not USART PRS input mode. */
	usartPrsRxCh0,     /* PRS channel 0. */
	false,              /* No AUTOTX mode. */
#if EFR32MG1P132F256GM48 || EFR32MG12P332F1024GL125
	false,				/* No AUTOCS mode. */
	0,					/* Auto CS Hold cycles */
	0					/* Auto CS Setup cycles */
#endif
};
#endif

void modSPIInit(modSPIConfiguration config)
{
	if (!gSPIInited) {
		gSPIInited = 1;
	}

//	CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1; // turn on clock for USART1
	CMU_ClockEnable(SPI_CLOCK, true);

	// Enable the GPIO pins for the USART, starting with CS
	// This is to avoid clocking the flash chip when we set CLK high
// CS port will be handled by whoever instantiates it
	GPIO_PinModeSet(MODDEF_SPI_MOSI_PORT, MODDEF_SPI_MOSI_PIN, gpioModePushPull, 0);
	GPIO_PinModeSet(MODDEF_SPI_MISO_PORT, MODDEF_SPI_MISO_PIN, gpioModeInput, 0);
	GPIO_PinModeSet(MODDEF_SPI_SCK_PORT, MODDEF_SPI_SCK_PIN, gpioModePushPull, 0);

	spiInit.enable = usartEnable;
	spiInit.refFreq = 0;
	spiInit.baudrate = config->hz;
	spiInit.databits = usartDatabits8;
	spiInit.master = true;
	spiInit.msbf = true;
	spiInit.prsRxEnable = false;
	spiInit.autoTx = false;
	
	switch (config->mode) {
		case 3:
			spiInit.clockMode = usartClockMode2;   // CPOL = 1, CPHA = 1
			break;
		case 2:
			spiInit.clockMode = usartClockMode3;   // CPOL = 1, CPHA = 0
			break;
		case 1:
			spiInit.clockMode = usartClockMode1;   // CPOL = 0, CPHA = 1
			break;
		case 0:
		default:
			spiInit.clockMode = usartClockMode0;   // CPOL = 0, CPHA = 0
			break;
	}
	
#if (MIGHTY_GECKO || THUNDERBOARD2 || BLUE_GECKO) // EFR32MG1P132F256GM48 || EFR32MG12P332F1024GL125
	spiInit.autoCsEnable = false;
	spiInit.autoCsHold = false;
	spiInit.autoCsSetup = false;
#endif
	USART_InitSync(SPI_USART, &spiInit);

	config->clkdiv = SPI_USART->CLKDIV;  // store calculated speed for easy switch

#if (MIGHTY_GECKO || THUNDERBOARD2 || BLUE_GECKO)// EFR32MG1P132F256GM48 || EFR32MG12P332F1024GL125
	SPI_USART->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_CLKPEN;
	SPI_USART->ROUTELOC0  = ( SPI_USART->ROUTELOC0 & ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK | _USART_ROUTELOC0_CLKLOC_MASK ) );
	SPI_USART->ROUTELOC0 |= ( MODDEF_SPI_MOSI_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT );
	SPI_USART->ROUTELOC0 |= ( MODDEF_SPI_MISO_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT );
	SPI_USART->ROUTELOC0 |= ( MODDEF_SPI_SCK_LOCATION << _USART_ROUTELOC0_CLKLOC_SHIFT );
#elif GIANT_GECKO // EFM32GG990F1024
	// Connect the USART signals to the GPIO peripheral
	SPI_USART->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN |
		USART_ROUTE_CLKPEN | /* USART_ROUTE_CSPEN | */
		MODDEF_SPI_LOCATION << _USART_ROUTE_LOCATION_SHIFT;
#else
	#error need SPI pins for new gecko part
#endif

	NVIC_EnableIRQ(SPI_TX_IRQ);
	(config->doChipSelect)(0, config);
}

void modSPIUninit(modSPIConfiguration config)
{
	modSPIActivateConfiguration(NULL);
}

void modSPIActivateConfiguration(modSPIConfiguration config)
{
	modSPIFlush();

// This is commented out to allow the Accelerometer SPI to work
// with the moddable driven display
	if (config == gConfig)
		return;

	if (gConfig)
		(gConfig->doChipSelect)(0, gConfig);

	gConfig = config;
	if (gConfig) {
//		USART1->CLKDIV = config->clkdiv;
		USART_BaudrateSyncSet(SPI_USART, 0, config->hz);
		// @@ if there is more than one SPI device and they don't all use the same mode,
		//	the mode needs to be updated here (clockMode bits in usart->CTRL)
		(gConfig->doChipSelect)(1, gConfig);
	}
}

// data must be long aligned
uint16_t modSpiLoadBufferAsIs(uint8_t *data, uint16_t bytes)
{
	uint32_t *from = (uint32_t *)data, *to = gSPITransactionBuffer;

	if (64 == bytes) {
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
		*to   = *from;
	}
	else {
		uint8_t longs = (bytes + 3) >> 2;
		while (longs--)
			*to++ = *from++;
	}

	return bytes;
}

uint16_t modSpiLoadBufferSwap16(uint8_t *data, uint16_t bytes)
{
	uint32_t *from = (uint32_t *)data;
	uint32_t *to = gSPITransactionBuffer;
	const uint32_t mask = 0x00ff00ff;
	uint32_t twoPixels;

	if (64 == bytes) {
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
		twoPixels = *from  ; *to   = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
	}
	else {
		uint8_t longs = (bytes + 3) >> 2;
		while (longs--) {
			twoPixels = *from++; *to++ = ((twoPixels & mask) << 8) | ((twoPixels >> 8) & mask);
		}
	}

	return bytes;

}

uint16_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint16_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer;
	uint16_t remain;

	if (bytes > (SPI_BUFFER_SIZE >> 1))
		bytes = SPI_BUFFER_SIZE >> 1;

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

uint16_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint16_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = gSPITransactionBuffer;
	uint16_t i;

	if (bytes > (SPI_BUFFER_SIZE >> 2))
		bytes = SPI_BUFFER_SIZE >> 2;

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

// N.B. callers assume this funtion is synchronous
void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	int i;

	modSPIActivateConfiguration(config);

	for (i=0; i<count; i++) {
		USART_Tx(SPI_USART, data[i]);
		data[i] = USART_Rx(SPI_USART);
	}
}

static volatile int16_t gSPIDataCount = -1;
static uint8_t *gSPIData;
static modSPIBufferLoader gSPIBufferLoader;

void modSPIFlush(void)
{
	while (gSPIDataCount >= 0)
		geckoEnterEM1();
//		EMU_EnterEM1();
}

static uint8_t *spiTxBuffer;
static volatile uint16_t spiTxBufferDataSize = 0;

void modSPIStartSend(uint16_t dataSize) {
	spiTxBuffer = (uint8_t*)gSPITransactionBuffer;
	spiTxBufferDataSize = dataSize;
	USART_IntClear(SPI_USART, _USART_IF_MASK);
	NVIC_ClearPendingIRQ(SPI_TX_IRQ);
//	NVIC_EnableIRQ(SPI_TX_IRQ);

	USART_IntEnable(SPI_USART, USART_IF_TXBL);
}

#if MODDEF_SPI_INTERFACE_USART == 0
void USART0_TX_IRQHandler(void)
#elif MODDEF_SPI_INTERFACE_USART == 1
void USART1_TX_IRQHandler(void)
#elif MODDEF_SPI_INTERFACE_USART == 2
void USART2_TX_IRQHandler(void)
#endif
{
	uint16_t loaded;
	if (SPI_USART->STATUS & USART_STATUS_TXBL) {
		if (spiTxBufferDataSize != 0) {
			SPI_USART->TXDATA = (uint32_t)*spiTxBuffer++;
				spiTxBufferDataSize--;
		}
		else {
			if (gSPIDataCount <= 0) {
				gSPIDataCount = -1;
				USART_IntDisable(SPI_USART, USART_IF_TXBL);
				return;
			}
			loaded = gSPIBufferLoader(gSPIData, (gSPIDataCount <= SPI_BUFFER_SIZE) ? gSPIDataCount : SPI_BUFFER_SIZE);
			gSPIDataCount -= loaded;
			gSPIData += loaded;
			modSPIStartSend(loaded);
		}
	}

}

static void modSPITxCommon(modSPIConfiguration config, uint8_t *data, uint16_t count, modSPIBufferLoader loader)
{
	uint16_t loaded;

	modSPIActivateConfiguration(config);

	loaded = (loader)(data, (count <= SPI_BUFFER_SIZE) ? count : SPI_BUFFER_SIZE);
	if (loaded >= count)
		gSPIDataCount = 0;
	else {
		gSPIBufferLoader = loader;
		gSPIData = data + loaded;
		gSPIDataCount = count - loaded;
	}
	modSPIStartSend(loaded);
}

void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferAsIs);
}

void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, modSpiLoadBufferSwap16);
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
	modLog_transmit("need to implement CLUT16to16BE\n");
}

