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
#define ESP32 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_bt.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "driver/uart.h"

#include "modInstrumentation.h"
#include "esp_system.h"		// to get system_get_free_heap_size, etc.

#include "xs.h"
#include "xsesp.h"

#include "xsPlatform.h"

extern void fx_putc(void *refcon, char c);		//@@
extern void mc_setup(xsMachine *the);

static xsMachine *gThe;		// the one XS virtual machine running

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
static QueueHandle_t gUARTQueue;

static void debug_task(void *pvParameter)
{
	while (true) {
		uart_event_t event;

		if (!xQueueReceive(gUARTQueue, (void * )&event, portMAX_DELAY))
			continue;

		if (UART_DATA == event.type)
			fxReceiveLoop();
	}
}
#endif

void setup(void)
{
	esp_err_t err;
	uart_config_t uartConfig;
#ifdef mxDebug
	uartConfig.baud_rate = 921600;
#else
	uartConfig.baud_rate = 115200;
#endif
	uartConfig.data_bits = UART_DATA_8_BITS;
	uartConfig.parity = UART_PARITY_DISABLE;
	uartConfig.stop_bits = UART_STOP_BITS_1;
	uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uartConfig.rx_flow_ctrl_thresh = 120;

	err = uart_param_config(USE_UART, &uartConfig);
	if (err)
		printf("uart_param_config err %d\n", err);
	err = uart_set_pin(USE_UART, USE_UART_TX, USE_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err)
		printf("uart_set_pin err %d\n", err);

#ifdef mxDebug
	err = uart_driver_install(USE_UART, 512, 512, 8, &gUARTQueue, 0);
#else
	err = uart_driver_install(USE_UART, 512, 512, 0, NULL, 0);
#endif
	if (err)
		printf("uart_driver_install err %d\n", err);

#ifdef mxDebug
	xTaskCreate(debug_task, "debug", 1024, NULL, 5, NULL);
#endif

	gThe = ESP_cloneMachine(0, 0, 0, NULL);

	mc_setup(gThe);
}

void loop_task(void *pvParameter)
{
	setup();

	while (true) {
		modTimersExecute();
		modMessageService(gThe, modTimersNext());
	}
}

/*
	Required functions provided by application
	to enable serial port for diagnostic information and debugging
*/

void modLog_transmit(const char *msg)
{
	uint8_t c;

	if (gThe) {
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else {
		while (0 != (c = c_read8(msg++)))
			ESP_putc(c);
		ESP_putc(13);
		ESP_putc(10);
	}
}

void ESP_putc(int c) {
	char cx = c;
	uart_write_bytes(USE_UART, &cx, 1);
}

int ESP_getc(void) {
	int err;
	uint8_t myChar = ' ';
	if (!ESP_isReadable())
		return -1;
	err = uart_read_bytes(USE_UART, &myChar, 1, 1);
	if (err == 1)
		return myChar;
	return -1;
}

uint8_t ESP_isReadable() {
	size_t s;
	uart_get_buffered_data_len(USE_UART, &s);
	return s > 0;
}

void app_main() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	esp_log_level_set("wifi", CONFIG_LOG_DEFAULT_LEVEL);

	nvs_flash_init();
    tcpip_adapter_init();
#if CONFIG_BT_ENABLED
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif
    ESP_ERROR_CHECK( esp_event_loop_init(NULL, NULL) );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack ((6 * 1024) / sizeof(StackType_t))
	#else
		#define kStack ((8 * 1024) / sizeof(StackType_t))
	#endif
    xTaskCreate(loop_task, "main", kStack, NULL, 4, NULL);
}
