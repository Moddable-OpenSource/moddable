/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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
#include "xsHost.h"
#include "modGPIO.h"
#include "mc.defines.h"

#ifndef MODDEF_MY92X1_DI_PIN
	#define MODDEF_MY92X1_DI_PIN (12)
#endif
#ifndef MODDEF_MY92X1_DCKI_PIN
	#define MODDEF_MY92X1_DCKI_PIN (14)
#endif

// For Sonoff B1: W C 0 G R B.

#undef LOW
#define LOW (0)
#undef HIGH
#define HIGH (1)

typedef struct {
	modGPIOConfigurationRecord 	di;
	modGPIOConfigurationRecord	dcki;
} my92x1Record, *my92x1;

static void my92x1Pulse(modGPIOConfiguration gpio, uint8_t times);
static void my92x1Write(my92x1 myl, uint8_t data);

void xs_LightMy92x1_destructor(void *data)
{
	my92x1 myl = data;
	if (!myl) return;

	modGPIOUninit(&myl->di);
	modGPIOUninit(&myl->dcki);

	c_free(myl);
}

void xs_LightMy92x1(xsMachine *the)
{
	my92x1 myl = c_calloc(1, sizeof(my92x1Record));
	xsmcSetHostData(xsThis, myl);

	modGPIOInit(&myl->di, NULL, MODDEF_MY92X1_DI_PIN, kModGPIOOutput);
	modGPIOInit(&myl->dcki, NULL, MODDEF_MY92X1_DCKI_PIN, kModGPIOOutput);

	modGPIOWrite(&myl->di, LOW);
	modGPIOWrite(&myl->dcki, LOW);

	my92x1Pulse(&myl->dcki, 2 * 32);           // Clear all duty register
	modDelayMicroseconds(12);                      // TStop > 12us.
	// Send 12 DI pulse, after 6 pulse's falling edge store duty data, and 12
	// pulse's rising edge convert to command mode.
	my92x1Pulse(&myl->di, 12);
	modDelayMicroseconds(12);                      // Delay >12us, begin send CMD data

	my92x1Write(myl, 0x18);				// ONE_SHOT_DISABLE, REACTION_FAST, BIT_WIDTH_8, FREQUENCY_DIVIDE_1, SCATTER_APDM
	my92x1Write(myl, 0x18);				// ONE_SHOT_DISABLE, REACTION_FAST, BIT_WIDTH_8, FREQUENCY_DIVIDE_1, SCATTER_APDM

	modDelayMicroseconds(12);                      // TStart > 12us. Delay 12 us.
	// Send 16 DI pulse, at 14 pulse's falling edge store CMD data, and
	// at 16 pulse's falling edge convert to duty mode.
	my92x1Pulse(&myl->di, 16);
	modDelayMicroseconds(12);                      // TStop > 12us.
}

void xs_LightMy92x1_write(xsMachine *the)
{
	my92x1 myl = xsmcGetHostData(xsThis);
	int argc = xsmcArgc, i;
	uint32_t values[16];

	if (argc > 16)
		xsUnknownError("too many arugments");

	for (i = 0; i < argc; i++)
		values[i] = xsmcToInteger(xsArg(i));

	for (i = 0; i < argc; i++)
		my92x1Write(myl, values[i]);

	modDelayMicroseconds(12);                      // TStart > 12us. Ready for send DI pulse.

	my92x1Pulse(&myl->di, 8);                      // Send 8 DI pulse. After 8 pulse falling edge, store old data.
	modDelayMicroseconds(12);                      // TStop > 12us.
}

void my92x1Pulse(modGPIOConfiguration gpio, uint8_t times)
{
	while (times--) {
		modGPIOWrite(gpio, HIGH);
		modGPIOWrite(gpio, LOW);
	}
}

void my92x1Write(my92x1 myl, uint8_t data)
{
	for (uint8_t i = 0; i < 4; i++) {
		modGPIOWrite(&myl->dcki, LOW);

		modGPIOWrite(&myl->di, (data & 0x80) ? HIGH : LOW);
		modGPIOWrite(&myl->dcki, HIGH);
		data = data << 1;

		modGPIOWrite(&myl->di, (data & 0x80) ? HIGH : LOW);
		modGPIOWrite(&myl->dcki, LOW);
		modGPIOWrite(&myl->di, LOW);
		data = data << 1;
	}
}
