/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xsesp.h"

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

#if 0

#if EFR32MG1P132F256GM48		// thunderboard
	#define SPI_PORT	gpioPortC
	#define SPI_MOSI	6
	#define SPI_MISO	7
	#define SPI_SCK		8
	#define SPI_CS		9
	#define SPI_LOCATION	11
	#define SPI_USART	USART1
	#define SPI_CLOCK	cmuClock_USART1
#elif EFM32GG990F1024			// giant gecko
	#define SPI_PORT	gpioPortD
	#define SPI_MOSI	0
	#define SPI_MISO	1
	#define SPI_SCK		2
	#define SPI_CS		3
//	#define SPI_RST		4
//	#define SPI_DC		5
	#define SPI_USART	USART1
	#define SPI_CLOCK	cmuClock_USART1
#elif EFR32MG12P332F1024GL125	// radio board
	#define SPI_PORT	gpioPortA
	#define SPI_MOSI	6
	#define SPI_MISO	7
	#define SPI_SCK		8
	#define SPI_CS		9
	#define SPI_LOCATION	1
	#define SPI_USART	USART2
	#define SPI_CLOCK	cmuClock_USART2
#else
	#error need SPI pins for new gecko part
#endif

#endif

#if !defined(MODDEF_SPI_PORT)
	#define MODDEF_SPI_PORT	0
#endif

#if MODDEF_SPI_PORT == 0
	#define SPI_PORT	USART0
	#define SPI_CLOCK	cmuClock_USART0
	#define SPI_TX_IRQ	USART0_TX_IRQn
#elif MODDEF_SPI_PORT == 1
	#define SPI_PORT	USART1
	#define SPI_CLOCK	cmuClock_USART1
	#define SPI_TX_IRQ	USART1_TX_IRQn
#elif MODDEF_SPI_PORT == 2
	#define SPI_PORT	USART2
	#define SPI_CLOCK	cmuClock_USART2
	#define SPI_TX_IRQ	USART2_TX_IRQn
#else
	#error bad SPI port
#endif

typedef uint16_t (*modSPIBufferLoader)(uint8_t *data, uint16_t bytes);

static uint16_t modSpiLoadBufferAsIs(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferSwap16(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint16_t bytes);
static uint16_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint16_t bytes);

static modSPIConfiguration gConfig;
static uint16_t *gCLUT16;		//@@ unused

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
	48000000,		/* 1000000,           /* 1 Mbits/s. */
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
	GPIO_PinModeSet(MODDEF_SPI_CS_PORT, MODDEF_SPI_CS_PIN, gpioModePushPull, 1);
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
#if EFR32MG1P132F256GM48 || EFR32MG12P332F1024GL125
	spiInit.autoCsEnable = false;
	spiInit.autoCsHold = false;
	spiInit.autoCsSetup = false;
#endif
	USART_InitSync(SPI_PORT, &spiInit);

	config->clkdiv = SPI_PORT->CLKDIV;  // store calculated speed for easy switch

#if EFR32MG1P132F256GM48 || EFR32MG12P332F1024GL125
	SPI_PORT->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_CLKPEN;
	SPI_PORT->ROUTELOC0  = ( SPI_PORT->ROUTELOC0 & ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK | _USART_ROUTELOC0_CLKLOC_MASK ) );
	SPI_PORT->ROUTELOC0 |= ( MODDEF_SPI_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT );
	SPI_PORT->ROUTELOC0 |= ( MODDEF_SPI_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT );
	SPI_PORT->ROUTELOC0 |= ( MODDEF_SPI_LOCATION << _USART_ROUTELOC0_CLKLOC_SHIFT );
#elif EFM32GG990F1024
	// Connect the USART signals to the GPIO peripheral
	SPI_PORT->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN |
		USART_ROUTE_CLKPEN | /* USART_ROUTE_CSPEN | */
		MODDEF_SPI_LOCATION;
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
		USART_BaudrateSyncSet(SPI_PORT, 0, config->hz);
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
		USART_Tx(SPI_PORT, data[i]);
		data[i] = USART_Rx(SPI_PORT);
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
	spiTxBuffer = gSPITransactionBuffer;
	spiTxBufferDataSize = dataSize;
	USART_IntClear(SPI_PORT, _USART_IF_MASK);
	NVIC_ClearPendingIRQ(SPI_TX_IRQ);
//	NVIC_EnableIRQ(SPI_TX_IRQ);

	USART_IntEnable(SPI_PORT, USART_IF_TXBL);
}

#if MODDEF_SPI_PORT == 0
void USART0_TX_IRQHandler(void)
#elif MODDEF_SPI_PORT == 1
void USART1_TX_IRQHandler(void)
#elif MODDEF_SPI_PORT == 2
void USART2_TX_IRQHandler(void)
#endif
{
	uint16_t loaded;
	if (SPI_PORT->STATUS & USART_STATUS_TXBL) {
		if (spiTxBufferDataSize != 0) {
			SPI_PORT->TXDATA = (uint32_t)*spiTxBuffer++;
				spiTxBufferDataSize--;
		}
		else {
			if (gSPIDataCount <= 0) {
				gSPIDataCount = -1;
				USART_IntDisable(SPI_PORT, USART_IF_TXBL);
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
	uint16_t i, loaded, bitsLoaded;

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
	printf("need to implement CLUT16to16BE\n");
}

