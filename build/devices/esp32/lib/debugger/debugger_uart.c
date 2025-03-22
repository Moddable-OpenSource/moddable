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

#include "driver/uart.h"

#include "xs.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "mc.defines.h"

#if MODDEF_ECMA419_ENABLED
	#include "common/builtinCommon.h"
#endif

// #define WEAK __attribute__((weak))
#define WEAK

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 460800
#endif

#if ESP32 == 1			// esp32
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	1
	#define USE_UART_RX	3
#elif ESP32 == 2		// esp32s2
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	43
	#define USE_UART_RX	44
#elif ESP32 == 3		// esp32s3
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	43
	#define USE_UART_RX	44
#elif ESP32 == 4		// esp32c3
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	21
	#define USE_UART_RX	20
#elif ESP32 == 5		// esp32c6
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	16
	#define USE_UART_RX	17
#elif ESP32 == 6		// esp32h2
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	24
	#define USE_UART_RX	23
#endif


extern void fx_putc(void *refcon, char c);		//@@
extern void mc_setup(xsMachine *the);

#ifndef UART_HW_FIFO_LEN
	#define UART_HW_FIFO_LEN(USE_UART) UART_FIFO_LEN
#endif

#ifdef mxDebug
	extern xsMachine *gThe;		// copied in from main
#endif

static void debug_task(void *pvParameter)
{
	while (true) {

		uart_event_t event;

		if (!xQueueReceive((QueueHandle_t)pvParameter, (void * )&event, portMAX_DELAY))
			continue;

		if (UART_DATA == event.type)
			fxReceiveLoop();
	}
}

/*
	Required functions provided by application
	to enable serial port for diagnostic information and debugging
*/

WEAK void modLog_transmit(const char *msg)
{
	uint8_t c;

#ifdef mxDebug
	if (gThe) {
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else
#endif
	{
		while (0 != (c = c_read8(msg++)))
			ESP_putc(c);
		ESP_putc(13);
		ESP_putc(10);
	}
}

WEAK void ESP_put(uint8_t *c, int count) {
	uart_write_bytes(USE_UART, (char *)c, count);
}

WEAK void ESP_putc(int c) {
	char cx = c;
	uart_write_bytes(USE_UART, &cx, 1);
}

WEAK int ESP_getc(void) {
	uint8_t c;
	int amt = uart_read_bytes(USE_UART, &c, 1, 0);
	return (1 == amt) ? c : -1;
}

WEAK uint8_t ESP_isReadable() {
	size_t s;
	uart_get_buffered_data_len(USE_UART, &s);
	return s > 0;
}

WEAK uint8_t ESP_setBaud(int baud) {
	uart_wait_tx_done(USE_UART, 5 * 1000);
	return ESP_OK == uart_set_baudrate(USE_UART, baud);
}

void setupDebugger(void)
{
	uart_config_t uartConfig = {0};

#ifdef mxDebug
	uartConfig.baud_rate = DEBUGGER_SPEED;
#else
	uartConfig.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
#endif
	uartConfig.data_bits = UART_DATA_8_BITS;
	uartConfig.parity = UART_PARITY_DISABLE;
	uartConfig.stop_bits = UART_STOP_BITS_1;
	uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uartConfig.rx_flow_ctrl_thresh = 120;		// unused. no hardware flow control.
	uartConfig.source_clk = UART_SCLK_DEFAULT;

#ifdef mxDebug
	QueueHandle_t uartQueue;
	uart_driver_install(USE_UART, UART_HW_FIFO_LEN(USE_UART) * 2, 0, 8, &uartQueue, 0);
#else
	uart_driver_install(USE_UART, UART_HW_FIFO_LEN(USE_UART) * 2, 0, 0, NULL, 0);
#endif

	uart_param_config(USE_UART, &uartConfig);
	uart_set_pin(USE_UART, USE_UART_TX, USE_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

#ifdef mxDebug
	xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), uartQueue, 8, NULL);
#endif

#if MODDEF_ECMA419_ENABLED
	builtinUsePin(USE_UART_TX);
	builtinUsePin(USE_UART_RX);
#endif
}
