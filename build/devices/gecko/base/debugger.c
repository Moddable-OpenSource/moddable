/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include "em_gpio.h"
#include "em_cmu.h"

#include "xs.h"
#include "xsgecko.h"
#include "xsPlatform.h"

#include "mc.defines.h"

#if mxDebug
#include "em_device.h"
#include "em_usart.h"
#include "em_leuart.h"
#include "em_system.h"

int gDebuggerSetup = 0;

#ifndef MODDEF_DEBUGGER_TX_LOCATION
	#define MODDEF_DEBUGGER_TX_LOCATION MODDEF_DEBUGGER_LOCATION
#endif
#ifndef MODDEF_DEBUGGER_RX_LOCATION
	#define MODDEF_DEBUGGER_RX_LOCATION MODDEF_DEBUGGER_LOCATION
#endif
#ifndef MODDEF_DEBUGGER_BAUD
	#define MODDEF_DEBUGGER_BAUD	115200
#endif

#define Q(x) #x
#define QUOTE(x) Q(x)

#if defined(MODDEF_DEBUGGER_INTERFACE_LEUART)
	#if (MODDEF_DEBUGGER_INTERFACE_LEUART == 0)
		#define DEBUGGER_PORT	LEUART0
		#define DEBUGGER_CLOCK	cmuClock_LEUART0
		#define DEBUGGER_INIT_CLOCK() CMU_ClockEnable(cmuClock_CORELE, true); CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO); CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1); CMU_ClockEnable(cmuClock_LEUART0, true);

	#elif (MODDEF_DEBUGGER_INTERFACE_LEUART == 1)
		#define DEBUGGER_PORT	LEUART1
		#define DEBUGGER_CLOCK	cmuClock_LEUART1
		#define DEBUGGER_INIT_CLOCK() CMU->LFCLKSEL &= ~_CMU_LFCLKSEL_LFB_MASK; CMU->LFCLKSEL |= CMU_LFCLKSEL_LFBE_ULFRCO; CMU->LFBCLKEN0 |= CMU_LFBCLKEN0_LEUART1;
	#endif
#elif defined(MODDEF_DEBUGGER_INTERFACE_UART)
	#if (MODDEF_DEBUGGER_INTERFACE_UART == 0)
		#define DEBUGGER_PORT	UART0
		#define DEBUGGER_CLOCK	cmuClock_UART0
		#define DEBUGGER_INIT_CLOCK() CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_UART0;
	#elif (MODDEF_DEBUGGER_INTERFACE_UART == 1)
		#define DEBUGGER_PORT	UART1
		#define DEBUGGER_CLOCK	cmuClock_UART1
		#define DEBUGGER_INIT_CLOCK() CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_UART1;
	#endif
#elif defined(MODDEF_DEBUGGER_INTERFACE_USART)
	#if (MODDEF_DEBUGGER_INTERFACE_USART == 0)
		#define DEBUGGER_PORT	USART0
		#define DEBUGGER_CLOCK	cmuClock_USART0
		#define DEBUGGER_INIT_CLOCK() CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART0;
	#elif (MODDEF_DEBUGGER_INTERFACE_USART == 1)
		#define DEBUGGER_PORT	USART1
		#define DEBUGGER_CLOCK	cmuClock_USART1
		#define DEBUGGER_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_ULFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1;
	#elif (MODDEF_DEBUGGER_INTERFACE_USART == 2)
		#define DEBUGGER_PORT	USART2
		#define DEBUGGER_CLOCK	cmuClock_USART2
		#define DEBUGGER_INIT_CLOCK() CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART2;
	#endif
#else
	#error unknown debugger interface
#endif

void modLog_transmit(const char *msg)
{
	uint8_t c;
	if (gThe) {
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 13);
		fx_putc(gThe, 10);
		fx_putc(gThe, 0);
	}
	else {
		while (0 != (c = c_read8(msg++)))
			ESP_putc(c);
		ESP_putc(13);
		ESP_putc(10);
		ESP_putc(0);
	}
}

void ESP_putc(int c) {
    while( !(DEBUGGER_PORT->STATUS & (1 << 6)) ); // wait for TX buffer to empty
//    while(USART_GetFlagStatus(DEBUGGER_PORT, USART_FLAG_TXE) == RESET ); // wait for TX buffer to empty
    DEBUGGER_PORT->TXDATA = c;       // print each character of the test string
}

int ESP_getc(void) {
	if (!ESP_isReadable())
		return -1;
	return DEBUGGER_PORT->RXDATA;
}

uint8_t ESP_isReadable() {
	if (DEBUGGER_PORT->STATUS & (1<<7))
		return 1;
//	if (USART_GetFlagStatus(DEBUGGER_PORT, USART_FLAG_RXNE) == SET)
//		return 1;
	return 0;
}
#else
void modLog_transmit(const char *msg) { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
#endif

void setupDebugger() {
#if mxDebug
#if MIGHTY_GECKO || BLUE_GECKO || THUNDERBOARD2
	USART_InitAsync_TypeDef serialInit = USART_INITASYNC_DEFAULT;
	CMU_ClockEnable(DEBUGGER_CLOCK, true);

	GPIO_PinModeSet(MODDEF_DEBUGGER_TX_PORT, MODDEF_DEBUGGER_TX_PIN, gpioModePushPull, 1);
	GPIO_PinModeSet(MODDEF_DEBUGGER_RX_PORT, MODDEF_DEBUGGER_RX_PIN, gpioModeInput, 0);

	serialInit.baudrate = MODDEF_DEBUGGER_BAUD;

	USART_InitAsync(DEBUGGER_PORT, &serialInit);

	DEBUGGER_PORT->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
	DEBUGGER_PORT->ROUTELOC0  = ( DEBUGGER_PORT->ROUTELOC0 & ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK ) );
	DEBUGGER_PORT->ROUTELOC0 |= ( MODDEF_DEBUGGER_TX_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT );
	DEBUGGER_PORT->ROUTELOC0 |= ( MODDEF_DEBUGGER_RX_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT );

	gDebuggerSetup = 1;

#elif GIANT_GECKO
	CMU_ClockEnable(DEBUGGER_CLOCK, true);
	DEBUGGER_INIT_CLOCK();

	GPIO_PinModeSet(MODDEF_DEBUGGER_TX_PORT, MODDEF_DEBUGGER_TX_PIN, gpioModePushPull, 1);
	GPIO_PinModeSet(MODDEF_DEBUGGER_RX_PORT, MODDEF_DEBUGGER_RX_PIN, gpioModeInput, 0);

#if defined(MODDEF_DEBUGGER_INTERFACE_LEUART)
	LEUART_Init_TypeDef serialInit = LEUART_INIT_DEFAULT;
	serialInit.baudrate = MODDEF_DEBUGGER_BAUD;
	LEUART_Init(DEBUGGER_PORT, &serialInit);
#elif defined(MODDEF_DEBUGGER_INTERFACE_UART) || defined(MODDEF_DEBUGGER_INTERFACE_USART)
	USART_InitAsync_TypeDef serialInit = USART_INITASYNC_DEFAULT;
	serialInit.baudrate = MODDEF_DEBUGGER_BAUD;
	USART_InitAsync(DEBUGGER_PORT, &serialInit);
#else
	modLog("unknown debugger PORT");
#endif

	DEBUGGER_PORT->ROUTE = (MODDEF_DEBUGGER_LOCATION << _USART_ROUTE_LOCATION_SHIFT)
			| USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;

	gDebuggerSetup = 1;
#else
	#error undefined gecko platform
#endif

#endif
}

