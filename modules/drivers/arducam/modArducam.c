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

/*
	ArduCAM Mini driver

	https://github.com/ArduCAM/ArduCAM_ESP8266_UNO/blob/master/libraries/ArduCAM/ArduCAM.cpp
	https://github.com/ArduCAM/ArduCAM_ESP8266_UNO/blob/b68a6dad3965503fcc25f137cbc5dd9681e037e4/libraries/ArduCAM/examples/Shield_V2/ArduCAM_Camera_Playback/ArduCAM_Camera_Playback.ino
	http://qyx.krtko.org/projects/ov2640_stm32/

	N.B. The ArduCAM device depends on CS toggling between each SPI register reads and write. That is done
	by calling modSPIActivateConfiguration with NULL to deselect the active SPI device after each
	SPI register operation completes.

*/

#include "xsmc.h"

#include "xsHost.h"
#include "modSPI.h"
#include "modI2C.h"
#include "modGPIO.h"
#include "mc.defines.h"

#include "mc.xs.h"			// for xsID_ values

#ifndef MODDEF_ARDUCAM_HZ
	#define MODDEF_ARDUCAM_HZ (12000000)
#endif
#ifndef MODDEF_ARDUCAM_I2C_SDA
	#define MODDEF_ARDUCAM_I2C_SDA (4)
#endif
#ifndef MODDEF_ARDUCAM_I2C_SCL
	#define MODDEF_ARDUCAM_I2C_SCL (5)
#endif
#ifndef MODDEF_ILI9341_CS_PORT
	#define MODDEF_ILI9341_CS_PORT NULL
#endif
#ifndef MODDEF_ILI9341_CS_PIN
	#define MODDEF_ILI9341_CS_PIN (16)
#endif

struct sensor_reg8 {
	uint8_t reg;
	uint8_t value;
};
typedef struct sensor_reg8 sensor_reg8;

#include "ov2640_regs.h"

static void arducamChipSelect(uint8_t active, modSPIConfiguration config);

typedef struct {
	modSPIConfigurationRecord spiConfig;
	modI2CConfigurationRecord i2cConfig;

	int		bytesAvailable;
	uint8_t	swap16;
	uint8_t	skipFirst;
	uint8_t	startBurst;

	modGPIOConfigurationRecord csPin;
} ArduCAMRecord, *ArduCAM;

static void writeRegisterSPI_8(ArduCAM ac, uint8_t reg, uint8_t value);
static uint8_t readRegisterSPI_FIFO(ArduCAM ac);
static uint8_t readRegisterSPI_8(ArduCAM ac, uint8_t reg);
static uint8_t readRegisterSPI_1(ArduCAM ac, uint8_t addr, uint8_t bit);

static void writeRegisterI2C_8(ArduCAM ac, uint8_t reg, uint8_t value);
static void writeRegistersI2C_8(ArduCAM ac, const struct sensor_reg8 *pairs);

#define ARDUCHIP_FIFO (4)
#define FIFO_CLEAR_MASK (1)
#define FIFO_START_MASK (2)
#define FIFO_RDPTR_RST_MASK (16)
#define FIFO_WRPTR_RST_MASK (32)

#define ARDUCHIP_TRIG (0x41)
#define VSYNC_MASK (1)
#define SHUTTER_MASK (2)
#define CAP_DONE_MASK (8)

#define FIFO_SIZE1				0x42
#define FIFO_SIZE2				0x43
#define FIFO_SIZE3				0x44

#define ARDUCHIP_MODE (2)
#define MCU2LCD_MODE (0)
#define CAM2LCD_MODE (1)
#define LCD2MCU_MODE (2)

#define BURST_FIFO_READ (0x3C)
#define SINGLE_FIFO_READ (0x3D)

#define startCapture() writeRegisterSPI_8(ac, ARDUCHIP_FIFO, FIFO_START_MASK)
#define clearFIFOFlag() writeRegisterSPI_8(ac, ARDUCHIP_FIFO, FIFO_CLEAR_MASK)

// MOSI, MISO, SCK as per HSPI on ESP8266
// CHIP SELECT on GPIO 16

void xs_arducam_destructor(void *data)
{
	if (data)
		free(data);
}

void xs_arducam(xsMachine *the)
{
	ArduCAM ac;
	int width, height;
	char *format;
	uint8_t value;

	ac = calloc(1, sizeof(ArduCAMRecord));
	if (!ac)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, ac);

	ac->spiConfig.hz = MODDEF_ARDUCAM_HZ;
	ac->spiConfig.doChipSelect = arducamChipSelect;

	/*
		I2C set-up first
	*/

	ac->i2cConfig.sda = MODDEF_ARDUCAM_I2C_SDA;
	ac->i2cConfig.scl = MODDEF_ARDUCAM_I2C_SCL;
	ac->i2cConfig.address = 0x30;				// OV2640 (ArcuCAM sources use 0x60 and then shift down by 1)
	modI2CInit(&ac->i2cConfig);

	writeRegisterI2C_8(ac, 0xff, 0x01);

	value = 0x0A;
	modI2CWrite(&ac->i2cConfig, &value, 1, true);
	modI2CRead(&ac->i2cConfig, &value, 1, true);
	if (0x26 != value)
		xsUnknownError("bad id high");

	value = 0x0B;
	modI2CWrite(&ac->i2cConfig, &value, 1, true);
	modI2CRead(&ac->i2cConfig, &value, 1, true);
	if ((0x41 != value ) && (0x42 != value))
		xsUnknownError("bad id low");

	writeRegisterI2C_8(ac, 0x12, 0x80);
	modDelayMilliseconds(100);

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_width);
	width = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_height);
	height = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_format);
	format = xsmcToString(xsVar(0));

	if (0 == c_strcmp(format, "rgb565be")) {
		if ((320 != width) || (240 != height))
			xsUnknownError("unsupported dimensions");

		writeRegistersI2C_8(ac, OV2640_QVGA);
	}
	else if (0 == c_strcmp(format, "rgb565le")) {
		if ((320 != width) || (240 != height))
			xsUnknownError("unsupported dimensions");

		writeRegistersI2C_8(ac, OV2640_QVGA);
		ac->swap16 = 1;
	}
	else if (0 == c_strcmp(format, "jpeg")) {
		// configure JPEG mode
		writeRegistersI2C_8(ac, OV2640_JPEG_INIT);
		writeRegistersI2C_8(ac, OV2640_YUV422);
		writeRegistersI2C_8(ac, OV2640_JPEG);
		writeRegisterI2C_8(ac, 0xff, 0x01);
		writeRegisterI2C_8(ac, 0x15, 0x00);
		if ((160 == width) && (120 == height))
			writeRegistersI2C_8(ac, OV2640_160x120_JPEG);
		else if ((176 == width) && (144 == height))
			writeRegistersI2C_8(ac, OV2640_176x144_JPEG);
		else if ((320 == width) && (240 == height))
			writeRegistersI2C_8(ac, OV2640_320x240_JPEG);
		else if ((352 == width) && (288 == height))
			writeRegistersI2C_8(ac, OV2640_352x288_JPEG);
		else if ((800 == width) && (600 == height))
			writeRegistersI2C_8(ac, OV2640_800x600_JPEG);
		else if ((1024 == width) && (768 == height))
			writeRegistersI2C_8(ac, OV2640_1024x768_JPEG);
		else if ((1280 == width) && (1024 == height))
			writeRegistersI2C_8(ac, OV2640_1280x1024_JPEG);
		else if ((1600 == width) && (1200 == height))
			writeRegistersI2C_8(ac, OV2640_1600x1200_JPEG);
		else
			xsUnknownError("unsupported dimensions");
	}
	else
		xsUnknownError("unrecognized format");

	/*
		SPI set-up second
	*/

	modGPIOInit(&ac->csPin, MODDEF_ILI9341_CS_PORT, MODDEF_ILI9341_CS_PIN, kModGPIOOutput);
	arducamChipSelect(false, ac);

	modSPIInit(&ac->spiConfig);

	// check chip ID on SPI
	writeRegisterSPI_8(ac, 0 /* == ARDUCHIP_TEST1 */, 0x55);
	if (0x55 != readRegisterSPI_8(ac, 0 /* == ARDUCHIP_TEST1 */))
		xsUnknownError("can't validate arducam on SPI bus");

	// MCU writes to screen (camera does not write to screen)
	writeRegisterSPI_8(ac, ARDUCHIP_MODE, MCU2LCD_MODE);
}

void xs_arducam_close(xsMachine *the)
{
	ArduCAM ac = xsmcGetHostData(xsThis);
	if (ac)
		xs_arducam_destructor(ac);
	xsmcSetHostData(xsThis, NULL);
}

void xs_arducam_capture(xsMachine *the)
{
	ArduCAM ac = xsmcGetHostData(xsThis);

	clearFIFOFlag();
	startCapture();

	while (0 == readRegisterSPI_1(ac, ARDUCHIP_TRIG, CAP_DONE_MASK))
		;

	ac->startBurst = 1;
	ac->skipFirst = 1;		// first byte in FIFO is not part of the image data stream

	// read FIFO length from camera
	ac->bytesAvailable = readRegisterSPI_8(ac, FIFO_SIZE1);
	ac->bytesAvailable |= readRegisterSPI_8(ac, FIFO_SIZE2) << 8;
	ac->bytesAvailable |= (readRegisterSPI_8(ac, FIFO_SIZE3) & 0x7f) << 16;

	xsmcSetInteger(xsResult, ac->bytesAvailable);
}

void xs_arducam_read(xsMachine *the)
{
	ArduCAM ac = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	int i;
	uint8_t data[4];
	uint8_t *buffer;
	int bufferSize = xsmcGetArrayBufferLength(xsArg(0));
	int readSize;

	if (1 == argc)
		readSize = bufferSize;
	else {
		readSize = xsmcToInteger(xsArg(1));
		if (readSize > bufferSize)
			readSize = bufferSize;
	}

	if (readSize > ac->bytesAvailable)
		readSize = ac->bytesAvailable;

	buffer = xsmcToArrayBuffer(xsArg(0));

#if 0
	if (ac->skipFirst) {
		readRegisterSPI_FIFO(ac);
		ac->skipFirst = 0;
	}

	for (i = 0; i < readSize; i += 1)
		*buffer++ = readRegisterSPI_FIFO(ac);
#else
	if (ac->startBurst) {
		data[0] = BURST_FIFO_READ;
		modSPITx(&ac->spiConfig, data, 1);
		ac->startBurst = 0;
	}

	if (ac->skipFirst) {
		uint8_t dummy = 0xff;
		modSPITxRx(&ac->spiConfig, &dummy, 1);
		ac->skipFirst = 0;
	}

	for (i = 0; i < readSize; ) {
		uint32_t temp32[64 / 4];
		uint8_t *temp8 = (uint8_t *)temp32;
		int use = readSize - i;
		if (use > 64) use = 64;

		modSPITxRx(&ac->spiConfig, temp8, use);

		if (ac->swap16) {
			uint8_t j;
			for (j = 0; j < use; j+= 2) {
				uint8_t temp = temp8[j];
				temp8[j] = temp8[j + 1];
				temp8[j + 1] = temp;
			}
		}

		c_memcpy(buffer, temp8, use);
		buffer += use;
		i += use;
	}
#endif

	ac->bytesAvailable -= readSize;
	if (0 == ac->bytesAvailable)
		clearFIFOFlag();

	xsmcSetInteger(xsResult, readSize);
}

void writeRegisterSPI_8(ArduCAM ac, uint8_t reg, uint8_t value)
{
	uint8_t data[2];

	data[0] = reg | 0x80;
	data[1] = value;
	modSPITx(&ac->spiConfig, data, 2);

	modSPIActivateConfiguration(NULL);
}

uint8_t readRegisterSPI_FIFO(ArduCAM ac)
{
	uint8_t data[2];

	data[0] = SINGLE_FIFO_READ;
	data[1] = 0;
	modSPITxRx(&ac->spiConfig, data, 2);

	modSPIActivateConfiguration(NULL);

	return data[1];
}

uint8_t readRegisterSPI_8(ArduCAM ac, uint8_t reg)
{
	uint8_t data[2];

	data[0] = reg;
	data[1] = 0;
	modSPITxRx(&ac->spiConfig, data, 2);

	modSPIActivateConfiguration(NULL);

	return data[1];
}

uint8_t readRegisterSPI_1(ArduCAM ac, uint8_t addr, uint8_t bit)
{
	return readRegisterSPI_8(ac, addr) & bit;
}

void writeRegisterI2C_8(ArduCAM ac, uint8_t reg, uint8_t value)
{
	uint8_t values[2];

	values[0] = reg;
	values[1] = value;
	modI2CWrite(&ac->i2cConfig, values, 2, true);

	modDelayMilliseconds(1);
}

void writeRegistersI2C_8(ArduCAM ac, const struct sensor_reg8 *pairs)
{
	while ((255 != c_read8(&pairs->reg)) | (255 != c_read8(&pairs->value))) {
		writeRegisterI2C_8(ac, c_read8(&pairs->reg), c_read8(&pairs->value));
		pairs++;
	}
}

void arducamChipSelect(uint8_t active, modSPIConfiguration config)
{
	ArduCAM ac = (ArduCAM)config;

	if (active)
		modGPIOWrite(&ac->csPin, 0);
	else {
		modGPIOWrite(&ac->csPin, 1);
		ac->startBurst = 1;
	}
}
