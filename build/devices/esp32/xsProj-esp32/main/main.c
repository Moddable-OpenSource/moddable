/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
#define ESP32 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
// #include "esp_event_loop.h"
#include "esp_task_wdt.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if CONFIG_BT_ENABLED
	#include "esp_bt.h"
#endif

#include "driver/uart.h"

#include "modInstrumentation.h"
#include "esp_system.h"		// to get system_get_free_heap_size, etc.

#include "xs.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "mc.defines.h"

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 921600
#endif

extern void fx_putc(void *refcon, char c);		//@@
extern void mc_setup(xsMachine *the);

#if 0 == CONFIG_LOG_DEFAULT_LEVEL
	#define kStack (((8 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
#else
	#define kStack (((10 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
#endif

#if !MODDEF_XS_TEST
static
#endif
	xsMachine *gThe;		// the main XS virtual machine running

/*
	xsbug IP address

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

#if 0
	#define USE_UART	UART_NUM_2
	#define USE_UART_TX	17
	#define USE_UART_RX	16
#else
	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	1
	#define USE_UART_RX	3
#endif

#ifdef mxDebug

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
#endif

void loop_task(void *pvParameter)
{
#if CONFIG_ESP_TASK_WDT_EN
	esp_task_wdt_add(NULL);
#endif

	while (true) {
		gThe = modCloneMachine(NULL, NULL);

		modRunMachineSetup(gThe);

#if MODDEF_XS_TEST
		xsMachine *the = gThe;
		while (gThe) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}

		xsDeleteMachine(the);
#else
		while (true) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
#endif
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

void ESP_put(uint8_t *c, int count) {
//#ifdef mxDebug
	uart_write_bytes(USE_UART, (char *)c, count);
//#endif
}

void ESP_putc(int c) {
//	#ifdef mxDebug
	char cx = c;
	uart_write_bytes(USE_UART, &cx, 1);
//#endif

}

int ESP_getc(void) {
//#ifdef mxDebug
	uint8_t c;
	int err = uart_read_bytes(USE_UART, &c, 1, 0);
	return (1 == err) ? c : -1;
//#endif
return -1;
}

uint8_t ESP_isReadable() {
//#ifdef mxDebug
	size_t s;
	uart_get_buffered_data_len(USE_UART, &s);
	return s > 0;
//#endif
return 0;
}

uint8_t ESP_setBaud(int baud) {
	uart_wait_tx_done(USE_UART, 5 * 1000);
	return ESP_OK == uart_set_baudrate(USE_UART, baud);
}

void app_main() {
	modPrelaunch();

#if defined(CONFIG_LOG_DEFAULT_LEVEL) && (CONFIG_LOG_DEFAULT_LEVEL > 0)
	esp_log_level_set("wifi", ESP_LOG_ERROR);
	esp_log_level_set("phy_init", ESP_LOG_ERROR);
	esp_log_level_set("I2S", ESP_LOG_ERROR);
#endif

	ESP_ERROR_CHECK(nvs_flash_init());
#if CONFIG_BT_ENABLED
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif

	esp_err_t err;
	uart_config_t uartConfig = {0};
#ifdef mxDebug
	uartConfig.baud_rate = DEBUGGER_SPEED;
#else
	uartConfig.baud_rate = 115200;		//@@ different from ESP8266
#endif
	uartConfig.data_bits = UART_DATA_8_BITS;
	uartConfig.parity = UART_PARITY_DISABLE;
	uartConfig.stop_bits = UART_STOP_BITS_1;
	uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uartConfig.rx_flow_ctrl_thresh = 120;		// unused. no hardware flow control.
	uartConfig.source_clk = UART_SCLK_DEFAULT;
//	uartConfig.use_ref_tick = 0;	 // deprecated in 4.x
	uartConfig.source_clk = UART_SCLK_APB;

	err = uart_param_config(USE_UART, &uartConfig);
	if (err)
		printf("uart_param_config err %d\n", err);
	err = uart_set_pin(USE_UART, USE_UART_TX, USE_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err)
		printf("uart_set_pin err %d\n", err);

#ifdef mxDebug
	QueueHandle_t uartQueue;
	uart_driver_install(USE_UART, UART_FIFO_LEN * 2, 0, 8, &uartQueue, 0);
	xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), uartQueue, 8, NULL);
#else
	uart_driver_install(USE_UART, UART_FIFO_LEN * 2, 0, 0, NULL, 0);
#endif

	xTaskCreate(loop_task, "main", kStack, NULL, 4, NULL);
//	xTaskCreatePinnedToCore(loop_task, "main", kStack, NULL, 4, NULL, 0);
}
