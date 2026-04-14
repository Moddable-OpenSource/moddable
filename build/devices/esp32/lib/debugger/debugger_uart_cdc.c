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

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"

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

#ifndef UART_HW_FIFO_LEN
	#error unexpected
#endif

#ifdef mxDebug
	extern xsMachine *gThe;		// copied in from main
#endif

static uint8_t bufferReady = 1;
static uint8_t buffer0_position = 0, buffer0_available = 0;
static uint8_t buffer1_position = 0, buffer1_available = 0;
static uint8_t buffer0[128];
static uint8_t buffer1[128];

static int8_t isUART = 0;		// 0 CDC, 1 UART
static int8_t gProbing = 1;
static int8_t gCDCDisconnected = 0;

static const char *strstr_l(const char *src, int src_l, const char *search)
{
	const char *srcEnd = src + src_l;

	while (src < srcEnd) {
		const char *ap = src, *bp = search;

		while (ap < srcEnd) {
			uint8_t b = *bp++;
			if (!b)
				return src;

			if (*ap++ != b)
				break;
		}
		src++;
	}

	return NULL;
}

// while probing: buffer0 JTAG, buffer1 UART
static void debug_task(void *pvParameter)
{
	usb_serial_jtag_driver_config_t cfg = { .rx_buffer_size = 512, .tx_buffer_size = 512 };
	usb_serial_jtag_driver_install(&cfg);

	uint32_t endProbe = xTaskGetTickCount() + 2000;

	while ((((uint32_t)xTaskGetTickCount()) < endProbe) &&
			(buffer0_available < (sizeof(buffer0) - 1)) &&
			(buffer1_available < (sizeof(buffer1) - 1))) {
		buffer0_available += usb_serial_jtag_read_bytes(buffer0 + buffer0_available, sizeof(buffer0) - 1 - buffer0_available, 1);
		buffer0[buffer0_available] = 0;

		buffer1_available += uart_read_bytes(USE_UART, buffer1 + buffer1_available, sizeof(buffer1) - 1 - buffer1_available, 1);
		buffer1[buffer1_available] = 0;

		if (strstr_l((char *)buffer0, buffer0_available, "<?xs#")) {
			isUART = 0;
			bufferReady = 0;
			buffer0_position = 0;

			uart_driver_delete(USE_UART);

#if MODDEF_ECMA419_ENABLED
			builtinFreePin(USE_UART_TX);
			builtinFreePin(USE_UART_RX);
#endif
			break;
		}
		if (strstr_l((char *)buffer1, buffer1_available, "<?xs#")) {
			isUART = 1;
			bufferReady = 1;
			buffer1_position = 0;

			usb_serial_jtag_driver_uninstall();
			break;
		}
	}

	gProbing = 0;

	if (isUART) {
		while (true) {
			if (0 == bufferReady) {
				if (buffer1_available) {
					vTaskDelay(1);
					continue;
				}

				int amt = uart_read_bytes(USE_UART, buffer1, sizeof(buffer1), 1);
				if (0 == amt)
					continue;
				buffer1_position = 0;
				buffer1_available = (uint8_t)amt;
			}
			else {
				if (buffer0_available) {
					vTaskDelay(1);
					continue;
				}
					
				int amt = uart_read_bytes(USE_UART, buffer0, sizeof(buffer0), 1);
				if (0 == amt)
					continue;
				buffer0_position = 0;
				buffer0_available = (uint8_t)amt;
			}

	#ifdef mxDebug
			fxReceiveLoop();
	#endif
		}
	}
	else {
		while (true) {
			if (0 == bufferReady) {
				if (buffer1_available) {
					vTaskDelay(1);
					continue;
				}

				int amt = usb_serial_jtag_read_bytes(buffer1, sizeof(buffer1), 1);
				if (0 == amt)
					continue;
				buffer1_position = 0;
				buffer1_available = (uint8_t)amt;
			}
			else {
				if (buffer0_available) {
					vTaskDelay(1);
					continue;
				}

				int amt = usb_serial_jtag_read_bytes(buffer0, sizeof(buffer0), 1);
				if (0 == amt)
					continue;
				buffer0_position = 0;
				buffer0_available = (uint8_t)amt;
			}

	#ifdef mxDebug
			fxReceiveLoop();
	#endif
		}
	}
}

WEAK void modLog_transmit(const char *msg)
{
#ifdef mxDebug
	if (gThe) {
		uint8_t c;
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else
#endif
	{
		ESP_put((uint8_t *)msg, strlen(msg));
		ESP_put((uint8_t *)"\r\n", 2);
	}
}

WEAK void ESP_put(uint8_t *c, int count)
{
	if (gProbing || isUART)
		uart_write_bytes(USE_UART, (char *)c, count);
	if (gProbing || !isUART) {
		if (gCDCDisconnected) return;
		while (count > 0) {
			int sent = usb_serial_jtag_write_bytes(c, count, 10);
			if (sent <= 0) {
				gCDCDisconnected = 1;		// if write fails, we're doomed because we don't retry. It proabbly means the CDC connection is down, so stop using it
				break;
			}
			c += sent;
			count -= sent;
		}
	}
}

WEAK void ESP_putc(int c)
{
	uint8_t cx = (uint8_t)c;
	ESP_put(&cx, 1);
}

WEAK int ESP_getc(void)
{
	if (gProbing)
		return -1;

	if (0 == bufferReady) {
		if (buffer0_available) {
			buffer0_available--;
			return buffer0[buffer0_position++];
		}
		else if (buffer1_available) {
			bufferReady = 1;
			buffer1_available--;
			return buffer1[buffer1_position++];
		}
	}
	else {
		if (buffer1_available) {
			buffer1_available--;
			return buffer1[buffer1_position++];
		}
		else if (buffer0_available) {
			bufferReady = 0;
			buffer0_available--;
			return buffer0[buffer0_position++];
		}
	}

	return -1;
}

WEAK uint8_t ESP_isReadable()
{
	if (gProbing)
		return 0;
	return buffer0_available || buffer1_available;
}

WEAK uint8_t ESP_setBaud(int baud)
{
	if (isUART) {
		uart_wait_tx_done(USE_UART, 5 * 1000);
		return ESP_OK == uart_set_baudrate(USE_UART, baud);
	}
	return 1;
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

	uart_driver_install(USE_UART, UART_HW_FIFO_LEN(USE_UART) * 2, 0, 0, NULL, 0);

	uart_param_config(USE_UART, &uartConfig);
	uart_set_pin(USE_UART, USE_UART_TX, USE_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

#if MODDEF_ECMA419_ENABLED
	builtinUsePin(USE_UART_TX);
	builtinUsePin(USE_UART_RX);
#endif

	xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), NULL, 8, NULL);
}
