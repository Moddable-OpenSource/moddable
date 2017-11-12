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

extern "C" {
	#include "user_interface.h"		// to get system_soft_wdt_feed

	extern void fx_putc(void *refcon, char c);		//@@
}

#include "xs.h"
#include "xsesp.h"

#include "xsPlatform.h"

xsMachine *gThe;		// the one XS6 virtual machine running

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
static uart_t *gUART;

static const char gSetup[] ICACHE_RODATA_ATTR = "setup";
static const char gRequire[] ICACHE_RODATA_ATTR = "require";
static const char gWeak[] ICACHE_RODATA_ATTR = "weak";
static const char gMain[] ICACHE_RODATA_ATTR = "main";

extern "C" int16_t fxFindModule(xsMachine* the, uint16_t moduleID, xsSlot* slot);
void setup()
{
	const char *module;

	gUART = uart_init(UART0, 460800, SERIAL_8N1, SERIAL_FULL, 1);		// ESP8266 boots to 74880

	system_set_os_print(0);

	gThe = ESP_cloneMachine(0, 0, 0, 0);

	xsBeginHost(gThe);
		xsResult = xsString(gSetup);
		if (XS_NO_ID != fxFindModule(the, XS_NO_ID, &xsResult))
			module = gSetup;
		else
			module = gMain;

		xsResult = xsGet(xsGlobal, xsID(gRequire));
		xsResult = xsCall1(xsResult, xsID(gWeak), xsString(module));
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
	xsEndHost(gThe);
}


void loop(void)
{
	if (!gThe)
		return;

#ifdef mxDebug
	if (ESP_isReadable()) {
		if (triggerDebugCommand(gThe)) {
			if (modTimersNextScript() > 500) {		// if a script is not likely to fire within half a second, break immediately
				xsBeginHost(gThe);
				xsDebugger();
				xsEndHost(gThe);
			}
		}
	}
#endif

	modTimersExecute();

	if (modRunPromiseJobs(gThe))
		return;

	modMessageService();

	int delayMS = modTimersNext();
	if (delayMS)
		delay((delayMS < 5) ? delayMS : 5);
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
