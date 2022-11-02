/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	 xpt2046 touch controller driver

		based on datasheets:
			https://ldm-systems.ru/f/doc/catalog/HY-TFT-2,8/XPT2046.pdf
			http://www.ti.com/lit/ds/symlink/ads7843.pdf
		and technical note:
			http://www.ti.com/lit/an/sbaa036/sbaa036.pdf

		This driver operates the xpt2046 (/ads7843) in differential mode (see technical note).
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"
#include "modGPIO.h"
#include "modSPI.h"
#include "stddef.h"		// for offsetof macro

#ifndef MODDEF_XPT2046_ZTHRESHOLD
	#define MODDEF_XPT2046_ZTHRESHOLD (100)
#endif
#ifndef MODDEF_XPT2046_HZ
	#define MODDEF_XPT2046_HZ (1000000)
#endif
#ifndef MODDEF_XPT2046_CS_PORT
	#define MODDEF_XPT2046_CS_PORT NULL
#endif
#ifndef MODDEF_XPT2046_TOUCH_PORT
	#define MODDEF_XPT2046_TOUCH_PORT NULL
#endif
#ifndef MODDEF_XPT2046_HISTORYCOUNT
	#define MODDEF_XPT2046_HISTORYCOUNT (6)
#endif
#ifndef MODDEF_XPT2046_DEJITTERTHRESHOLD
	#define MODDEF_XPT2046_DEJITTERTHRESHOLD (30)
#endif
#ifndef MODDEF_XPT2046_RAW
	#define MODDEF_XPT2046_RAW (false)
#endif
#if MODDEF_XPT2046_RAW
	#define MODDEF_XPT2046_CALIBRATE (false)
#else
	#define MODDEF_XPT2046_CALIBRATE (true)
#endif

// bit 7 - start bit
// bit 6-4 A2-A0 - Channel select bits
// bit 3 - mode (low for 12bit, high for 8bit)
// bit 2 - single ended / differential reference
// bit 1-0 - power down mode (00 - enabled, 01 ref off, adc on, 10 ref on, adc off, 11-always on)
#define CTRLZ1 0b10110011 		// 0xB3 = 179				// 10110011
#define CTRLZ2 0b11000011 		// 0xC3 = 195				// 11000011
#define CTRLY  0b10010011 		// 0x93 = 147				// 10010011
#define CTRLX  0b11010011 		// 0xD3 = 211				// 11010011
#define CTRL_RESET 0b11010100	// 0xD4 = 212		// 11010100

struct xpt2046Record {
	modSPIConfigurationRecord spiConfig;

#if MODDEF_XPT2046_CALIBRATE
	int16_t			min_x;
	int16_t			max_x;
	int16_t			min_y;
	int16_t			max_y;
#endif

	uint16_t		priorX[MODDEF_XPT2046_HISTORYCOUNT];
	uint16_t		priorY[MODDEF_XPT2046_HISTORYCOUNT];
	uint8_t			priorCount;
	uint8_t			state;		// 1 = down, 2 = move, 3 = lift

#ifdef MODDEF_XPT2046_TOUCH_PIN
	modGPIOConfigurationRecord	touchPin;
#endif

	modGPIOConfigurationRecord	csPin;
};
typedef struct xpt2046Record xpt2046Record;
typedef xpt2046Record *xpt2046;

static void xpt2046ChipSelect(uint8_t active, modSPIConfiguration config);
static void powerDown(xpt2046 xpt);
static void xpt2046GetPosition(xpt2046 xpt, uint16_t *x, uint16_t *y);
static int xpt2046GetZ1(xpt2046 xpt);

void xs_XPT2046_destructor(void *data)
{
	xpt2046 xpt = data;
	if (xpt) {
		modSPIUninit(&xpt->spiConfig);
		c_free(data);
	}
}

void xs_XPT2046(xsMachine *the)
{
	xpt2046 xpt = c_calloc(1, sizeof(xpt2046Record));
	if (!xpt) xsUnknownError("out of memory");
	xsmcSetHostData(xsThis, xpt);

	modSPIConfig(xpt->spiConfig, MODDEF_XPT2046_HZ, MODDEF_XPT2046_SPI_PORT,
			MODDEF_XPT2046_CS_PORT, MODDEF_XPT2046_CS_PIN, xpt2046ChipSelect);

#ifdef MODDEF_XPT2046_MISO_DELAY
	xpt->spiConfig.miso_delay = MODDEF_XPT2046_MISO_DELAY;
#endif
	
	modGPIOInit(&xpt->csPin, MODDEF_XPT2046_CS_PORT, MODDEF_XPT2046_CS_PIN, kModGPIOOutput);
	modGPIOWrite(&xpt->csPin, 1);

#ifdef MODDEF_XPT2046_TOUCH_PIN
	modGPIOInit(&xpt->touchPin, MODDEF_XPT2046_TOUCH_PORT, MODDEF_XPT2046_TOUCH_PIN, kModGPIOInput);
#endif

	modSPIInit(&xpt->spiConfig);

	xpt->priorCount = 0;
	xpt->state = 0;

#if MODDEF_XPT2046_CALIBRATE
	xpt->min_x = MODDEF_XPT2046_RAW_LEFT;
	xpt->max_x = MODDEF_XPT2046_RAW_RIGHT;
	xpt->min_y = MODDEF_XPT2046_RAW_TOP;
	xpt->max_y = MODDEF_XPT2046_RAW_BOTTOM;

	xsmcVars(1);
	xsVar(0) = xsCall0(xsThis, xsID_calibrate);
	if (xsmcTest(xsVar(0))) {
		uint16_t *calibration = xsmcToArrayBuffer(xsVar(0));
		if (xsmcGetArrayBufferLength(xsVar(0)) >= 8) {
			xpt->min_x = calibration[0];
			xpt->max_x = calibration[1];
			xpt->min_y = calibration[2];
			xpt->max_y = calibration[3];
		}
	}
#endif

	powerDown(xpt);
}

#define DISTANCE(x1, x2) (((int)x1 - (int)x2) * ((int)x1 - (int)x2))

void xs_XPT2046_read(xsMachine *the)
{
	xpt2046 xpt = xsmcGetHostData(xsThis);
	uint16_t x, y, i;

	xsmcVars(2);
	xsmcGetIndex(xsVar(1), xsArg(0), 0);

#ifdef MODDEF_XPT2046_TOUCH_PIN
	if (modGPIORead(&xpt->touchPin))
#else
	if (xpt2046GetZ1(xpt) < MODDEF_XPT2046_ZTHRESHOLD)
#endif
	{
		if ((1 != xpt->state) && (2 != xpt->state)) {
			xpt->state = 0;
			xpt->priorCount = 0;
			goto setState;
		}
		xpt->state = 3;
	}

	if (xpt->priorCount) {
		if (xpt->priorCount >= 3) {
			int thisDelta = DISTANCE(xpt->priorX[xpt->priorCount - 1], xpt->priorX[xpt->priorCount - 3]) + DISTANCE(xpt->priorY[xpt->priorCount - 1], xpt->priorY[xpt->priorCount - 3]);
			int prevDelta = DISTANCE(xpt->priorX[xpt->priorCount - 2], xpt->priorX[xpt->priorCount - 3]) + DISTANCE(xpt->priorY[xpt->priorCount - 2], xpt->priorY[xpt->priorCount - 3]);
			if ((prevDelta > MODDEF_XPT2046_DEJITTERTHRESHOLD) && (thisDelta < prevDelta)) {
				xpt->priorX[xpt->priorCount - 2] = xpt->priorX[xpt->priorCount - 3];
				xpt->priorY[xpt->priorCount - 2] = xpt->priorY[xpt->priorCount - 3];
			}
			else if (prevDelta < MODDEF_XPT2046_DEJITTERTHRESHOLD) {
					xpt->priorX[xpt->priorCount - 2] = xpt->priorX[xpt->priorCount - 3];
					xpt->priorY[xpt->priorCount - 2] = xpt->priorY[xpt->priorCount - 3];
			}
		}

		x = xpt->priorX[0], y = xpt->priorY[0];
		for (i = 1; i < xpt->priorCount - 1; i++) {
			x += xpt->priorX[i];
			y += xpt->priorY[i];
		}
		if (xpt->priorCount > 2) {
			x /= (xpt->priorCount - 1);
			y /= (xpt->priorCount - 1);
		}

		xsmcSetInteger(xsVar(0), x);
		xsmcSet(xsVar(1), xsID_x, xsVar(0));
		xsmcSetInteger(xsVar(0), y);
		xsmcSet(xsVar(1), xsID_y, xsVar(0));
		if ((0 == xpt->state) || (1 == xpt->state))
			xpt->state += 1;
	}

	xpt2046GetPosition(xpt, &x, &y);
	if (xpt->priorCount == MODDEF_XPT2046_HISTORYCOUNT) {
		for (i = 1; i < MODDEF_XPT2046_HISTORYCOUNT; i++) {
			xpt->priorX[i - 1] = xpt->priorX[i];
			xpt->priorY[i - 1] = xpt->priorY[i];
		}
		xpt->priorCount = MODDEF_XPT2046_HISTORYCOUNT - 1;
	}

	xpt->priorX[xpt->priorCount] = x;
	xpt->priorY[xpt->priorCount] = y;
	xpt->priorCount += 1;

setState:
	xsmcSetInteger(xsVar(0), xpt->state);
	xsmcSet(xsVar(1), xsID_state, xsVar(0));
}

void xpt2046ChipSelect(uint8_t active, modSPIConfiguration config)
{
	xpt2046 xpt = (xpt2046)(((char *)config) - offsetof(xpt2046Record, spiConfig));

	modGPIOWrite(&xpt->csPin, active ? 0 : 1);
}

void xpt2046GetPosition(xpt2046 xpt, uint16_t *x, uint16_t *y)
{
	int16_t sample;
	int32_t zero = 0;

	powerDown(xpt);

	sample = CTRLX;
	modSPITxRx(&xpt->spiConfig, (uint8_t *)&sample, sizeof(sample));

	sample = CTRLY;
	modSPITxRx(&xpt->spiConfig, (uint8_t *)&sample, sizeof(sample));

	powerDown(xpt);

	sample = CTRLX;
	modSPITxRx(&xpt->spiConfig, (uint8_t *)&sample, sizeof(sample));
	*x = sample >> 4;

	sample = CTRLY;
	modSPITxRx(&xpt->spiConfig, (uint8_t *)&sample, sizeof(sample));
	*y = sample >> 4;

#if MODDEF_XPT2046_CALIBRATE
	*x = (*x - xpt->min_x) * ((float)MODDEF_XPT2046_WIDTH) / (xpt->max_x - xpt->min_x);
	*y = (*y - xpt->min_y) * ((float)MODDEF_XPT2046_HEIGHT) / (xpt->max_y - xpt->min_y);

	if (*x < 0) *x = 0;
	if (*x > (MODDEF_XPT2046_WIDTH - 1)) *x = MODDEF_XPT2046_WIDTH - 1;
	if (*y < 0) *y = 0;
	if (*y > (MODDEF_XPT2046_HEIGHT - 1)) *y = MODDEF_XPT2046_HEIGHT - 1;
#endif
#if MODDEF_XPT2046_FLIPX
	*x = MODDEF_XPT2046_WIDTH - *x;
#endif
#if MODDEF_XPT2046_FLIPY
	*y = MODDEF_XPT2046_HEIGHT - *y;
#endif


	modSPITxRx(&xpt->spiConfig, (uint8_t *)&zero, sizeof(zero));

	powerDown(xpt);		// reset to get interrupt pin working again
}

int xpt2046GetZ1(xpt2046 xpt)
{
	uint8_t data[3] = {CTRLZ1, 0, 0};
	modSPITxRx(&xpt->spiConfig, (uint8_t *)&data, sizeof(data));
	return (data[1] << 8 | data[2]) >> 3;
}

void powerDown(xpt2046 xpt)
{
	uint8_t data = CTRL_RESET;
	modSPITxRx(&xpt->spiConfig, &data, 1);
	modSPIActivateConfiguration(NULL);
}
