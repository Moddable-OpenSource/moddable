/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

#define __XS6PLATFORMMINIMAL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "driver/usb_serial_jtag.h"

#include "xs.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "mc.defines.h"

#ifndef XT_STACK_EXTRA
	#define XT_STACK_EXTRA  512
#endif

//#define WEAK __attribute__((weak))
#define WEAK

#ifdef mxDebug
	extern xsMachine *gThe;
#endif

extern void fx_putc(void *refcon, char c);		//@@

uint8_t jtagReady = 1;
uint8_t jtag0_position = 0, jtag0_available = 0;
uint8_t jtag1_position = 0, jtag1_available = 0;
static uint8_t jtag0[128];
static uint8_t jtag1[128];

static void debug_task(void *pvParameter)
{
	usb_serial_jtag_driver_config_t cfg = { .rx_buffer_size = 512, .tx_buffer_size = 512 };
	usb_serial_jtag_driver_install(&cfg);

	while (true) {
		if (0 == jtagReady) {
			int amt = usb_serial_jtag_read_bytes(jtag1, sizeof(jtag1), 1);
			if (0 == amt)
				continue;
			jtag1_position = 0;
			jtag1_available = (uint8_t)amt;  
		}
		else {
			int amt = usb_serial_jtag_read_bytes(jtag0, sizeof(jtag0), 1);
			if (0 == amt)
				continue;
			jtag0_position = 0;
			jtag0_available = (uint8_t)amt;  
		}

#ifdef mxDebug
		fxReceiveLoop();
#endif
	}
}

/*
	Required functions provided by application
	to enable serial port for diagnostic information and debugging
*/

WEAK void modLog_transmit(const char *msg)
{
#ifdef mxDebug
	if (gThe) {
		uint8_t c;
		while (0 != (c = *msg++))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else
#endif
	{
		const uint8_t crlf[] = {13, 10};
		ESP_put((uint8_t *)msg, strlen(msg));
		ESP_put((uint8_t *)crlf, 2);
	}
}

WEAK void ESP_put(uint8_t *c, int count) {
	while (count > 0) {
		int sent = usb_serial_jtag_write_bytes(c, count, 10);
		if (sent <= 0)
			break;
		c += sent;
		count -= sent;
	}
}

WEAK void ESP_putc(int c) {
	char cx = c;
	usb_serial_jtag_write_bytes(&cx, 1, 1);
}

WEAK int ESP_getc(void) {
	if (0 == jtagReady) {
		if (jtag0_available) {
			jtag0_available--;
			return jtag0[jtag0_position++];
		}
		else if (jtag1_available) {
			jtagReady = 1;
			jtag1_available--;
			return jtag1[jtag1_position++];
		}
	}
	else {
		if (jtag1_available) {
			jtag1_available--;
			return jtag1[jtag1_position++];
		}
		else if (jtag0_available) {
			jtagReady = 0;
			jtag0_available--;
			return jtag0[jtag0_position++];
		}
	}

	return -1;
}

WEAK uint8_t ESP_isReadable() {
	return jtag0_available || jtag1_available;
}

WEAK uint8_t ESP_setBaud(int baud) {
	return 1;
}

void setupDebugger(void) {
    xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), 0, 8, NULL);
}
