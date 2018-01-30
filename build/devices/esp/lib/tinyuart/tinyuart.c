/*
 uart.cpp - esp8266 UART HAL

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */

/*
	 Stripped version of uart.c from ESP8266 Arduino SDK version 2.4
	 For use with Moddable SDK *only*

	 Changes:
		 - uart0 only
		 - most supporting functions removed
		 - merged uart_rx_buffer_ and its buffer into uart_
		 - call esp_schedule from ISR to wakeup loop()
		 - remove NULL checks on uart
		 - ignore mode on uart_init (tx and rx always enabled)
		 - don't thow away data on overflow; instead disable interrupts until buffer space available

	 Unstripped public APIs unchanged to allow switching back to full implementation of uart.c

	 N.B. This version should only be used with the Moddable SDK as its invocation of esp_schedule conflicts
			 with some arduino functions (Wi-Fi, DNS resolve). It can cause Adruino delay() to exit
			 early (this is deliberate and used in main loop).

	 jph 1/27/2018
*/

#include "Arduino.h"
#include "uart.h"
#include "esp8266_peri.h"
#include "user_interface.h"

extern void esp_schedule(void);
static void ICACHE_RAM_ATTR uart_receive(uart_t* uart);

#define UART_NR (UART0)

struct uart_rx_buffer_ {
	uint16_t size;
	uint16_t rpos;
	uint16_t wpos;
	uint8_t buffer[1];
};

struct uart_ {
	uint8_t rx_pin;
	uint8_t tx_pin;
	struct uart_rx_buffer_ rx_buffer;
};

int uart_peek_char(uart_t* uart)
{
	if (!uart_rx_available(uart))
		return -1;
	return uart->rx_buffer.buffer[uart->rx_buffer.rpos];
}

int uart_read_char(uart_t* uart)
{
	int data = uart_peek_char(uart);
	if (-1 != data) {
		uint16_t rpos = uart->rx_buffer.rpos + 1;
		uart->rx_buffer.rpos = (rpos == uart->rx_buffer.size) ? 0 : rpos;
	}
	return data;
}

size_t uart_rx_available(uart_t* uart)
{
	size_t result;

	ETS_UART_INTR_DISABLE();

	uart_receive(uart);

	if (uart->rx_buffer.wpos < uart->rx_buffer.rpos)
		result = (uart->rx_buffer.wpos + uart->rx_buffer.size) - uart->rx_buffer.rpos;
	else
		result = uart->rx_buffer.wpos - uart->rx_buffer.rpos;
	ETS_UART_INTR_ENABLE();

	return result;
}

void ICACHE_RAM_ATTR uart_receive(uart_t* uart)
{
	while((USS(UART_NR) >> USRXC) & 0x7F){
		uint16_t nextPos = uart->rx_buffer.wpos + 1;
		if (nextPos == uart->rx_buffer.size)
			nextPos = 0;
		if (nextPos == uart->rx_buffer.rpos) {			// rx_buffer full
			ETS_UART_INTR_DISABLE();
			break;
		}
		uart->rx_buffer.buffer[uart->rx_buffer.wpos] = USF(UART_NR);
		uart->rx_buffer.wpos = nextPos;
	}
}

void ICACHE_RAM_ATTR uart_isr(void * arg)
{
	uart_t* uart = (uart_t*)arg;
	if (USIS(UART_NR) & ((1 << UIFF) | (1 << UITO))){
		uart_receive(uart);
		esp_schedule();
	}
	USIC(UART_NR) = USIS(UART_NR);
}

void uart_start_isr(uart_t* uart)
{
	USC1(UART_NR) = (120 << UCFFT) | (0x02 << UCTOT) | (1 <<UCTOE );		// generate interrupt after 120 bytes received or 2 character time recieve timeout
	USIC(UART_NR) = 0xffff;
	USIE(UART_NR) = (1 << UIFF) | (1 << UIFR) | (1 << UITO);
	ETS_UART_INTR_ATTACH(uart_isr,  (void *)uart);
	ETS_UART_INTR_ENABLE();
}

void uart_stop_isr(uart_t* uart)
{
	ETS_UART_INTR_DISABLE();
	USC1(UART_NR) = 0;
	USIC(UART_NR) = 0xffff;
	USIE(UART_NR) = 0;
	ETS_UART_INTR_ATTACH(NULL, NULL);
}


void uart_write_char(uart_t* uart, char c)
{
	while((USS(UART_NR) >> USTXC) >= 0x7f);
	USF(UART_NR) = c;
}

#if kESP8266Version >= 24
	uart_t* uart_init(int uart_nr, int baudrate, int config, int mode, int tx_pin,size_t rx_size)
#else
	uart_t* uart_init(int uart_nr, int baudrate, int config, int mode, int tx_pin)
#endif
{
#if kESP8266Version < 24
	size_t rx_size = 128;
#endif
	uart_t* uart = (uart_t*) malloc(sizeof(uart_t) + rx_size - 1);
	if (!uart)
		return NULL;

	ETS_UART_INTR_DISABLE();
	uart->rx_pin = 3;

	uart->rx_buffer.size = rx_size;
	uart->rx_buffer.rpos = 0;
	uart->rx_buffer.wpos = 0;
	pinMode(uart->rx_pin, SPECIAL);

	if (tx_pin == 2) {
		uart->tx_pin = 2;
		pinMode(uart->tx_pin, FUNCTION_4);
	} else {
		uart->tx_pin = 1;
		pinMode(uart->tx_pin, FUNCTION_0);
	}
	IOSWAP &= ~(1 << IOSWAPU0);

	USD(UART_NR) = (ESP8266_CLOCK / baudrate);
	USC0(UART_NR) = config;

	uint32_t tmp = (1 << UCRXRST) | (1 << UCTXRST);
	USC0(UART_NR) |= (tmp);
	USC0(UART_NR) &= ~(tmp);

	uart_start_isr(uart);

	return uart;
}

void uart_uninit(uart_t* uart)
{
	if (!uart)
		return;

	pinMode(uart->rx_pin, INPUT);
	pinMode(uart->tx_pin, INPUT);

	uart_stop_isr(uart);
	free(uart);
}
