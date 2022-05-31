/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "xsHost.h"
#include "xsHosts.h"
#include "modTimer.h"

extern "C" {
	#include "user_interface.h"		// to get system_soft_wdt_feed

	extern void fx_putc(void *refcon, char c);
	extern void __wrap_espconn_init(void);
	extern uint8_t uart_set_baud(uart_t* uart, int baudrate);
}

void __wrap_espconn_init(void)
{
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

	static xsMachine *gThe;		// root virtual machine
#endif

static uart_t *gUART;

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 921600
#endif

void setup()
{
#if kESP8266Version >= 24
	gUART = uart_init(UART0, DEBUGGER_SPEED, SERIAL_8N1, SERIAL_FULL, 1, 128);
#else
	gUART = uart_init(UART0, DEBUGGER_SPEED, SERIAL_8N1, SERIAL_FULL, 1);		// ESP8266 boots to 74880
#endif

	system_set_os_print(0);

	modPrelaunch();

	wifi_set_opmode_current(NULL_MODE);

#ifdef mxDebug
	gThe = modCloneMachine(0, 0, 0, 0, NULL);
	if (!gThe) {
		modLog("can't clone: no memory?");
		while (true)
			;
	}

	modRunMachineSetup(gThe);
#else
	modRunMachineSetup(modCloneMachine(0, 0, 0, 0, NULL));
#endif
}


void loop(void)
{
#if mxDebug
	fxReceiveLoop();
#endif

	modTimersExecute();

	if (0 == modMessageService()) {
		int delayMS = modTimersNext();
		if (delayMS)
			modDelayMilliseconds(delayMS);
	}
}

/*
	Required functions provided by application
	to enable serial port for diagnostic information and debugging
*/

void modLog_transmit(const char *msg)
{
	uint8_t c;

#ifdef mxDebug
	if (gThe) {
		while (c = c_read8(msg++))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else
#endif
	{
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

void ESP_put(uint8_t *c, int count)
{
	system_soft_wdt_feed();

	while (count--)
		uart_write_char(gUART, *c++);
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

uint8_t ESP_setBaud(int baud)
{
	return uart_set_baud(gUART, baud);
}
