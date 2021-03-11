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

#include "xs.h"
#include "xsPlatform.h"

#include "mc.defines.h"

#include "tusb.h"

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
	static int8_t lastRTS = -1;
	if (-1 != lastRTS) {
		if (lastRTS && !rts)
			pico_reboot(dtr);
	}
	lastRTS = rts;
}

#ifdef mxDebug

void tud_cdc_rx_cb(uint8_t itf)
{
	modMessagePostToMachineFromISR(NULL, (modMessageDeliver)fxReceiveLoop, NULL);
}

#endif

//---------
void setupDebugger()
{
	int i;

	for (i=0; i<=19; i++) {
		if (tud_cdc_connected()) {
			printf("pico-usb-connected.\r\n");
			break;
		}

		delay(500);
	}
}

void flushDebugger()
{
	stdio_flush();
}

void ESP_putc(int c)
{
	if (tud_cdc_connected())
		putchar(c);
}

int ESP_getc(void)
{
	if (!tud_cdc_connected())
		return -1;

	int c = getchar_timeout_us(0);
	return (PICO_ERROR_TIMEOUT == c) ? -1 : c;
}
