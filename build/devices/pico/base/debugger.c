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

#ifdef mxDebug

#ifndef MODDEF_DEBUGGER_RX_PIN
	#define MODDEF_DEBUGGER_RX_PIN	(0)
#endif
#ifndef MODDEF_DEBUGGER_TX_PIN
	#define MODDEF_DEBUGGER_TX_PIN	(1)
#endif
#ifndef MODDEF_DEBUGGER_BAUDRATE
	#define MODDEF_DEBUGGER_BAUDRATE	NRF_UART_BAUDRATE_115200
#endif

//---------
void setupDebugger()
{
#if mxDebug
	int i;

	for (i=1; i<=10; i++) {
		printf("pico-usb-start %d\r\n", i);
		delay(1000);
	}
#endif
}

void debuggerTask()
{
	fxReceiveLoop();
}

void flushDebugger()
{
	stdio_flush();
}

void ESP_putc(int c)
{
	putchar(c);
}

int ESP_getc(void)
{
	int c;
	c = getchar_timeout_us(1000);
	if (PICO_ERROR_TIMEOUT == c)
		return -1;
	return c;
}

#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }

#endif


