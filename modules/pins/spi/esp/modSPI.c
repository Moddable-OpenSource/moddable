/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "modSPI.h"
#include "spi_register.h"
#include "eagle_soc.h"        // CLEAR_PERI_REG_MASK, SET_PERI_REG_MASK

#include <ets_sys.h>
#include "string.h"

#include "xsHost.h"
#include "commodettoBitmap.h"

#define SPI (0)
#define HSPI (1)

#define kSPIEndianLittle (0)
#define kSPIEndianBig (1)

#define SPI_BUFFER_SIZE (64)
#define modSPIStartSend() SET_PERI_REG_MASK(SPI_FLASH_CMD(HSPI), SPI_FLASH_USR)

typedef uint8_t (*modSPIBufferLoader)(uint8_t *data, uint8_t bytes);

static uint8_t modSpiLoadBufferAsIs(uint8_t *data, uint8_t bytes);

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
	static uint8_t modSpiLoadBufferSwap16(uint8_t *data, uint8_t bytes);
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332
	static uint8_t modSpiLoadBufferRGB332To16BE(uint8_t *data, uint8_t bytes);
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
	static uint8_t modSpiLoadBufferGray256To16BE(uint8_t *data, uint8_t bytes);
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray16
	static uint8_t modSpiLoadBufferGray16To16BE(uint8_t *data, uint8_t bytes);
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	static uint8_t modSpiLoadBufferCLUT16To16BE(uint8_t *data, uint8_t bytes);
#endif

static uint32_t frequencyToSPIClock(uint32_t freq);

static void spiTxInterrupt(void *refcon);
static void modSPISetMode(uint8_t mode);

static modSPIConfiguration gConfig;
static uint8_t *gSPIData;
static volatile int16_t gSPIDataCount = -1;
static modSPIBufferLoader gSPIBufferLoader;
#if kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
	static uint16_t *gCLUT16;
#endif

#define SPI_CTRL1(i) (REG_SPI_BASE(i) + 0xc)
#define SPI_CS_HOLD_DELAY 0xf
#define SPI_CS_HOLD_DELAY_S 28
#define SPI_CS_HOLD_DELAY_RES 0xfff
#define SPI_CS_HOLD_DELAY_RES_S 16

void modSPIInit(modSPIConfiguration config)
{
	if (NULL == gConfig) {
		if ((13 != config->mosi_pin) || (12 != config->miso_pin) || (14 != config->clock_pin)) {
			modLog("invalid SPI");
			return;
		}

		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); // clear bit 9

		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); // HSPIQ MISO
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); // HSPID MOSI
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); // CLK

		gSPIDataCount = -1;
		SET_PERI_REG_MASK(SPI_FLASH_CMD(HSPI), SPI_FLASH_USR);

		SET_PERI_REG_BITS(SPI_CTRL1(HSPI), SPI_CS_HOLD_DELAY, 0, SPI_CS_HOLD_DELAY_S);

		ETS_SPI_INTR_ATTACH(spiTxInterrupt, NULL);
		ETS_SPI_INTR_ENABLE();
	}

	if (80000000 == config->hz) {
		#define hspi_enable_80Mhz()	 WRITE_PERI_REG(PERIPHS_IO_MUX, 0x305)
		hspi_enable_80Mhz();
		config->reserved = SPI_CLK_EQU_SYSCLK;
	}
	else
		config->reserved = frequencyToSPIClock(config->hz);
	
	modSPISetMode(config->mode);
}

void modSPIUninit(modSPIConfiguration config)
{
	modSPIActivateConfiguration(NULL);
	// disable SPI interrupt?
}

void modSPIActivateConfiguration(modSPIConfiguration config)
{
	modSPIFlush();

	if (config == gConfig)
		return;

	if (gConfig) {
		(gConfig->doChipSelect)(0, gConfig);
		if (!config) {
			gConfig = config;
			return;
		}
		if (config->reserved == gConfig->reserved) {
			gConfig = config;
			(gConfig->doChipSelect)(1, gConfig);
			return;
		}
	}

	gConfig = config;
	if (gConfig) {
		WRITE_PERI_REG(SPI_FLASH_CLOCK(HSPI), gConfig->reserved);

		(gConfig->doChipSelect)(1, gConfig);
	}
	
	if (config)
		modSPISetMode(config->mode);
}

// data must be long aligned
uint8_t ICACHE_RAM_ATTR modSpiLoadBufferAsIs(uint8_t *data, uint8_t bytes)
{
	uint32_t *from = (uint32_t *)data, *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits = (bytes << 3) - 1;

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

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

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
uint8_t ICACHE_RAM_ATTR modSpiLoadBufferSwap16(uint8_t *data, uint8_t bytes)
{
	uint32_t *from = (uint32_t *)data, *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits = (bytes << 3) - 1;
	uint32_t twoPixels;

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

	if (64 == bytes) {
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		twoPixels = *from  ; *to   = (twoPixels >> 16) | (twoPixels << 16);
	}
	else {
		uint8_t longs = (bytes + 3) >> 2;
		while (longs--) {
			twoPixels = *from++; *to++ = (twoPixels >> 16) | (twoPixels << 16);
		}
	}

	return bytes;
}

#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332

uint8_t ICACHE_RAM_ATTR modSpiLoadBufferRGB332To16BE(uint8_t *data, uint8_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits;
	uint16_t remain;

	if (bytes > (SPI_BUFFER_SIZE >> 1))
		bytes = SPI_BUFFER_SIZE >> 1;
	bits = (bytes << 4) - 1;		// output bits are double input bits

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

	remain = bytes;
	while (remain >= 2) {
		uint8_t rgb332, r, g, b;
		uint16_t pixela, pixelb;

		rgb332 = *from++;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixela = (r << 11) | (g << 5) | b;

		rgb332 = *from++;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixelb = (r << 11) | (g << 5) | b;

		*to++ = (pixela << 16) | pixelb;
		remain -= 2;
	}

	if (remain) {
		uint16_t pixela;
		uint8_t rgb332 = *from++, r, g, b;
		r = rgb332 >> 5, g = (rgb332 >> 2) & 0x07, b = rgb332 & 0x03;
		r = (r << 2) | (r >> 2), g |= g << 3, b = (b << 3) | (b << 1) | (b >> 1);
		pixela = (r << 11) | (g << 5) | b;
		*to++ = pixela << 16;
	}

	return bytes;		// input bytes consumed
}

#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
uint8_t ICACHE_RAM_ATTR modSpiLoadBufferGray256To16BE(uint8_t *data, uint8_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits;
	uint8_t remain;

	if (bytes > (SPI_BUFFER_SIZE >> 1))
		bytes = SPI_BUFFER_SIZE >> 1;
	bits = (bytes << 4) - 1;		// output bits are double input bits

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

	remain = bytes;
	while (remain >= 2) {
		uint8_t gray;
		uint16_t pixela, pixelb;

		gray = *from++;
		pixela = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);

		gray = *from++;
		pixelb = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);

		*to++ = (pixela << 16) | pixelb;
		remain -= 2;
	}

	if (remain) {
		uint8_t gray = *from++;
		uint16_t pixela = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);
		*to++ = pixela << 16;
	}

	return bytes;		// input bytes consumed
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray16
uint8_t ICACHE_RAM_ATTR modSpiLoadBufferGray16To16BE(uint8_t *data, uint8_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits;
	uint8_t i;

	if (bytes > (SPI_BUFFER_SIZE >> 2))
		bytes = SPI_BUFFER_SIZE >> 2;
	bits = (bytes << 5) - 1;		// output bits are quadruple input bits

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

	for (i = 0; i < bytes; i++) {
		uint8_t gray;
		uint8_t twoPixels = *from++;
		uint16_t pixela, pixelb;

		gray = (twoPixels >> 2) & 0x3C;
		gray |= gray >> 4;				// 6 bits of gray
		pixela = gray >> 1;				// 5 bita of gray
		pixela |= (pixela << 11) | (gray << 5);

		gray = (twoPixels << 2) & 0x3C;
		gray |= gray >> 4;
		pixelb = gray >> 1;
		pixelb |= (pixelb << 11) | (gray << 5);

		*to++ = (pixela << 16) | pixelb;
	}

	return bytes;		// input bytes consumed
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
uint8_t ICACHE_RAM_ATTR modSpiLoadBufferCLUT16To16BE(uint8_t *data, uint8_t bytes)
{
	uint8_t *from = (uint8_t *)data;
	uint32_t *to = (uint32_t *)SPI_FLASH_C0(HSPI);
	uint16_t bits;
	uint8_t i;
	volatile uint32_t *clut32_t = (uint32_t *)gCLUT16;

	if (bytes > (SPI_BUFFER_SIZE >> 2))
		bytes = SPI_BUFFER_SIZE >> 2;
	bits = (bytes << 5) - 1;		// output bits are quadruple input bits

	WRITE_PERI_REG(SPI_FLASH_USER1(HSPI),
			( (bits & SPI_USR_OUT_BITLEN) << SPI_USR_OUT_BITLEN_S ) |
			( (bits & SPI_USR_DIN_BITLEN) << SPI_USR_DIN_BITLEN_S ) );

	for (i = 0; i < bytes; i++) {
		uint8_t index;
		uint8_t twoPixels = *from++;
		uint32_t pixela;
		uint32_t t;

		index = twoPixels >> 4;
		t = clut32_t[index >> 1];
		if (index & 1)
			pixela = t & 0xFFFF0000;
		else
			pixela = t << 16;

		index = twoPixels & 0x0F;
		t = clut32_t[index >> 1];
		if (index & 1)
			*to++ = pixela | (t >> 16);
		else
			*to++ = pixela | (uint16_t)t;
	}

	return bytes;		// input bytes consumed
}
#endif

static void modSPIReadFromBuffer(uint8_t *data, uint8_t bytes)
{
	if (64 == bytes) {
		uint32_t *to = (uint32_t *)data, *from = (uint32_t *)SPI_FLASH_C0(HSPI);
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
//@@ optimize this code
		uint8_t i;
		uint8_t shift = 0;
		uint32_t buffer = 0;
		for (i = 0; i < bytes; ++i) {
			if ( (i % 4 == 0) ) {
				buffer = ( *( (uint32_t *)SPI_FLASH_C0(HSPI) + i/4 ) );
				shift = 0;
			}
			data[i] = (buffer >> shift) & 0xFF;
			shift += 8;
		}
	}
}

void modSPITxRx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	if (3 & (int)data) {
		modLog("SPI txrx data address not long aligned");
		return;
	}

	modSPIActivateConfiguration(config);
	gSPIBufferLoader = modSpiLoadBufferAsIs;

	WRITE_PERI_REG(SPI_FLASH_USER(HSPI), SPI_FLASH_DOUT | SPI_DOUTDIN | SPI_CK_I_EDGE);		// always lttle-endian

	modSpiLoadBufferAsIs(data, count);
	gSPIDataCount = 0;
	modSPIStartSend();
	modSPIFlush();
	modSPIReadFromBuffer(data, count);
}

void modSPIFlush(void)
{
	while (gSPIDataCount >= 0)
		;
}

static void modSPITxCommon(modSPIConfiguration config, uint8_t *data, uint16_t count, uint8_t endian, modSPIBufferLoader loader)
{
	uint8_t loaded;

	if (3 & (int)data) {
		modLog("SPI tx data address not long aligned");
		return;
	}

	modSPIActivateConfiguration(config);

	if (kSPIEndianLittle == endian)
		WRITE_PERI_REG(SPI_FLASH_USER(HSPI), SPI_FLASH_DOUT);
	else
		WRITE_PERI_REG(SPI_FLASH_USER(HSPI), SPI_FLASH_DOUT | SPI_WR_BYTE_ORDER);		// byte swap each 32-bit long as it is transmitted

	loaded = (loader)(data, (count <= SPI_BUFFER_SIZE) ? count : SPI_BUFFER_SIZE);
	if (loaded >= count)
		gSPIDataCount = 0;
	else {
		gSPIBufferLoader = loader;
		gSPIData = data + loaded;
		gSPIDataCount = count - loaded;
	}

	modSPIStartSend();

	if (config->sync)
		modSPIFlush();
}

void modSPITx(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, kSPIEndianLittle, modSpiLoadBufferAsIs);
}

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
void modSPITxSwap16(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, kSPIEndianBig, modSpiLoadBufferSwap16);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapRGB332
void modSPITxRGB332To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, kSPIEndianBig, modSpiLoadBufferRGB332To16BE);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray256
void modSPITxGray256To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, kSPIEndianBig, modSpiLoadBufferGray256To16BE);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapGray16
void modSPITxGray16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count)
{
	modSPITxCommon(config, data, count, kSPIEndianBig, modSpiLoadBufferGray16To16BE);
}
#elif kCommodettoBitmapFormat == kCommodettoBitmapCLUT16
void modSPITxCLUT16To16BE(modSPIConfiguration config, uint8_t *data, uint16_t count, uint16_t *colors)
{
	gCLUT16 = colors;		//@@ should wait for current transmit to finish before setting this
	modSPITxCommon(config, data, count, kSPIEndianBig, modSpiLoadBufferCLUT16To16BE);
}
#endif

/*
	clock calculations from Arduino for ESP code (actually, from Espressif SDK)
*/

#define ESP8266_CLOCK 80000000UL

typedef union {
        uint32_t regValue;
        struct {
                unsigned regL :6;
                unsigned regH :6;
                unsigned regN :6;
                unsigned regPre :13;
                unsigned regEQU :1;
        };
} spiClk_t;

static uint32_t ClkRegToFreq(spiClk_t * reg)
{
    return (ESP8266_CLOCK / ((reg->regPre + 1) * (reg->regN + 1)));
}

uint32_t frequencyToSPIClock(uint32_t freq)
{
    if (freq >= ESP8266_CLOCK)
        return 0x80000000;

    const spiClk_t minFreqReg = { 0x7FFFF000 };
    uint32_t minFreq = ClkRegToFreq((spiClk_t*) &minFreqReg);
    if(freq < minFreq)
        return minFreqReg.regValue;	// use minimum possible clock

    uint8_t calN = 1;

    spiClk_t bestReg = { 0 };
    int32_t bestFreq = 0;

    // find the best match
    while(calN <= 0x3F) { // 0x3F max for N

        spiClk_t reg = { 0 };
        int32_t calFreq;
        int32_t calPre;
        int8_t calPreVari = -2;

        reg.regN = calN;

        while(calPreVari++ <= 1) { // test different variants for Pre (we calculate in int so we miss the decimals, testing is the easyest and fastest way)
            calPre = (((ESP8266_CLOCK / (reg.regN + 1)) / freq) - 1) + calPreVari;
            if(calPre > 0x1FFF) {
                reg.regPre = 0x1FFF; // 8191
            } else if(calPre <= 0) {
                reg.regPre = 0;
            } else {
                reg.regPre = calPre;
            }

            reg.regL = ((reg.regN + 1) / 2);
            // reg.regH = (reg.regN - reg.regL);

            // test calculation
            calFreq = ClkRegToFreq(&reg);
            //os_printf("-----[0x%08X][%d]\t EQU: %d\t Pre: %d\t N: %d\t H: %d\t L: %d = %d\n", reg.regValue, freq, reg.regEQU, reg.regPre, reg.regN, reg.regH, reg.regL, calFreq);

            if(calFreq == (int32_t) freq) {
                // accurate match use it!
                memcpy(&bestReg, &reg, sizeof(bestReg));
                break;
            } else if(calFreq < (int32_t) freq) {
                // never go over the requested frequency
                if(abs(freq - calFreq) < abs(freq - bestFreq)) {
                    bestFreq = calFreq;
                    memcpy(&bestReg, &reg, sizeof(bestReg));
                }
            }
        }
        if(calFreq == (int32_t) freq) {
            // accurate match use it!
            break;
        }
        calN++;
    }

    // os_printf("[0x%08X][%d]\t EQU: %d\t Pre: %d\t N: %d\t H: %d\t L: %d\t - Real Frequency: %d\n", bestReg.regValue, freq, bestReg.regEQU, bestReg.regPre, bestReg.regN, bestReg.regH, bestReg.regL, ClkRegToFreq(&bestReg));

    return bestReg.regValue;
}


// http://bbs.espressif.com/viewtopic.php?t=360
// http://bbs.espressif.com/viewtopic.php?t=342

void ICACHE_RAM_ATTR spiTxInterrupt(void *refcon)
{
	if (READ_PERI_REG(0x3ff00020) & BIT7) {		// hspi isr
		uint32_t value = READ_PERI_REG(SPI_FLASH_SLAVE(HSPI));		//@@ unneeded code!?!
		uint8_t loaded;

// some of these mask manipulations may be unnecessary
		CLEAR_PERI_REG_MASK(SPI_FLASH_SLAVE(HSPI),
							SPI_TRANS_DONE_EN |
							SPI_SLV_WR_STA_DONE_EN |
							SPI_SLV_RD_STA_DONE_EN |
							SPI_SLV_WR_BUF_DONE_EN |
							SPI_SLV_RD_BUF_DONE_EN);
		SET_PERI_REG_MASK(SPI_FLASH_SLAVE(HSPI), SPI_SYNC_RESET);
		CLEAR_PERI_REG_MASK(SPI_FLASH_SLAVE(HSPI),
							SPI_TRANS_DONE |
							SPI_SLV_WR_STA_DONE |
							SPI_SLV_RD_STA_DONE |
							SPI_SLV_WR_BUF_DONE |
							SPI_SLV_RD_BUF_DONE); 
		SET_PERI_REG_MASK(SPI_FLASH_SLAVE(HSPI),
							SPI_TRANS_DONE_EN |
							SPI_SLV_WR_STA_DONE_EN |
							SPI_SLV_RD_STA_DONE_EN |
							SPI_SLV_WR_BUF_DONE_EN |
							SPI_SLV_RD_BUF_DONE_EN);


		if (gSPIDataCount <= 0) {
			gSPIDataCount = -1;
			return;
		}

		loaded = (gSPIBufferLoader)(gSPIData, (gSPIDataCount <= SPI_BUFFER_SIZE) ? gSPIDataCount : SPI_BUFFER_SIZE);
		gSPIDataCount -= loaded;
		gSPIData += loaded;

		modSPIStartSend();
	}
	else
	if (READ_PERI_REG(0x3ff00020) & BIT4)
		CLEAR_PERI_REG_MASK(SPI_FLASH_SLAVE(SPI), 0x3ff);
	else
	if (READ_PERI_REG(0x3ff00020) & BIT9)	// i2s isr
		;
}


void modSPISetMode(uint8_t mode)
{
	switch (mode) {
		case 3:
			CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
			SET_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);      // CPOL = 1, CPHA = 1
			break;
		case 2:
			SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
			SET_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);      // CPOL = 1, CPHA = 0
			break;
		case 1:
			SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
			CLEAR_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);    // CPOL = 0, CPHA = 1
			break;
		case 0:
		default:
			CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
			CLEAR_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);    // CPOL = 0, CPHA = 0
			break;
	}
}
