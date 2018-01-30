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

#define __XS6PLATFORMMINIMAL__

#include <Arduino.h>
#include "xs.h"
#include "xsesp.h"

extern "C" {
	#include "user_interface.h"		// to get system_soft_wdt_feed

	extern void fx_putc(void *refcon, char c);
	extern void mc_setup(xsMachine *the);
}

/*
	Wi-Fi configuration and xsbug IP address
		
	IP address either:
		0,0,0,0 - no xsbug connection
		127,0,0,7 - xsbug over serial
		w,x,y,z - xsbug over TCP (address of computer running xsbug)
*/

#define XSDEBUG_NONE 0,0,0,0
#define XSDEBUG_SERIAL 127,0,0,7
#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

#ifdef mxDebug
	unsigned char gXSBUG[4] = {DEBUG_IP};
#endif

static xsMachine *gThe;		// root virtual machine

static uart_t *gUART;

void setup()
{
#if kESP8266Version >= 24
	gUART = uart_init(UART0, 921600, SERIAL_8N1, SERIAL_FULL, 1, 128);
#else
	gUART = uart_init(UART0, 921600, SERIAL_8N1, SERIAL_FULL, 1);		// ESP8266 boots to 74880
#endif

	system_set_os_print(0);

	gThe = ESP_cloneMachine(0, 0, 0, NULL);

	mc_setup(gThe);
}


void loop(void)
{
	if (!gThe)
		return;

#if mxDebug
	fxReceiveLoop();
#endif

	modTimersExecute();

	modMessageService();

	int delayMS = modTimersNext();
	if (delayMS)
		modDelayMilliseconds((delayMS < 5) ? delayMS : 5);
}

/*
	Required functions provided by application
	to enable serial port for diagnostic information and debugging
*/

void modLog_transmit(const char *msg)
{
	uint8_t c;

	if (gThe) {
		while (c = c_read8(msg++))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else {
		while (c = c_read8(msg++))
			ESP_putc(c);
		ESP_putc(13);
		ESP_putc(10);
	}
}

void ESP_putc(int c)
{
	system_soft_wdt_feed();

	uart_write_char(gUART, c);
}

int ESP_getc(void)
{
	system_soft_wdt_feed();

	return uart_read_char(gUART);
}

uint8_t ESP_isReadable()
{
	return uart_rx_available(gUART);
}
