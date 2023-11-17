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
#define ESP32 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
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

#define WEAK __attribute__((weak))

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 921600
#endif

#if USE_USB
	#if (USE_USB == 1)
		#include "sdkconfig.h"
		#include "tinyusb.h"
		#include "tusb_cdc_acm.h"
	#elif (USE_USB == 2)
		#include "driver/usb_serial_jtag.h"
	#endif
#else
	#include "driver/uart.h"

	#define USE_UART	UART_NUM_0
	#define USE_UART_TX	43
	#define USE_UART_RX	44
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
	WEAK unsigned char gXSBUG[4] = {DEBUG_IP};
#endif

#if (USE_USB == 1)
typedef struct {
    uint8_t     *buf;
    uint32_t    size_mask;
    volatile uint32_t read;
    volatile uint32_t write;
} fifo_t;

QueueHandle_t usbDbgQueue;
static uint32_t usbEvtPending = 0;
static fifo_t rx_fifo;
static uint8_t *rx_fifo_buffer;
static uint8_t usb_rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

static uint32_t F_length(fifo_t *fifo) {
	uint32_t tmp = fifo->read;
	return fifo->write - tmp;
}

static void F_put(fifo_t *fifo, uint8_t c) {
	fifo->buf[fifo->write & fifo->size_mask] = c;
	fifo->write++;
}

static void F_get(fifo_t *fifo, uint8_t *c) {
	*c = fifo->buf[fifo->read & fifo->size_mask];
	fifo->read++;
}

/*
void fifo_flush(fifo_t *fifo) {
	fifo->read = fifo->write;
}
*/

uint32_t fifo_length(fifo_t *fifo) {
	return F_length(fifo);
}

uint32_t fifo_remain(fifo_t *fifo) {
	return (fifo->size_mask + 1) - F_length(fifo);
}

int fifo_get(fifo_t *fifo, uint8_t *c) {
	if (F_length(fifo) == 0)
		return -1;
	F_get(fifo, c);
	return 0;
}

int fifo_put(fifo_t *fifo, uint8_t c) {
	if (0 == (fifo->size_mask - F_length(fifo) + 1))
{
printf("fifo_put failed\r\n");
		return -1;
}
	F_put(fifo, c);
	return 0;
}

int fifo_init(fifo_t *fifo, uint8_t *buf, uint32_t size) {
	if (0 == buf)
		return -1;

	if (! ((0 != size) && (0 == ((size - 1) & size))))
{
printf("fifo_init - bad size: %ld\r\n", size);
		return -2;		// bad size - needs to be base 2
}

	fifo->buf = buf;
	fifo->size_mask = size - 1;
	fifo->read = 0;
	fifo->write = 0;

	return 0;
}

#endif

#ifdef mxDebug
static void debug_task(void *pvParameter)
{
#if (USE_USB == 2)
	const usb_serial_jtag_driver_config_t cfg = { .rx_buffer_size = 4096, .tx_buffer_size = 2048 };
	usb_serial_jtag_driver_install(&cfg);
#endif

	while (true) {

#if (USE_USB == 1)
		uint32_t count;
		if (!fifo_length(&rx_fifo)) {
			usbEvtPending = 0;
			xQueueReceive((QueueHandle_t)pvParameter, (void * )&count, portMAX_DELAY);
		}

		fxReceiveLoop();

#elif (USE_USB == 2)
		fxReceiveLoop();
		modDelayMilliseconds(5);

#else	// !USE_USB

		uart_event_t event;

		if (!xQueueReceive((QueueHandle_t)pvParameter, (void * )&event, portMAX_DELAY))
			continue;

		if (UART_DATA == event.type)
			fxReceiveLoop();
#endif	// !USE_USB
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
			if (gThe)
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

#if (USE_USB == 1)
static uint8_t DTR = 1;
static uint8_t RTS = 1;

void checkLineState() {
	uint8_t seq = (DTR ? 1 : 0) + (RTS ? 2 : 0);
	if (seq == 3) {				// normal run mode
		;
	}
	else if (seq == 2) {		// DTR dropped, RTS asserted
		esp_restart();
	}
	else if (seq == 1) {		// DTR raised, RTS off - programming mode
		;	// can't get here (we just reset)
	}
	else {
	}
}

void line_state_callback(int itf, cdcacm_event_t *event) {
	DTR = event->line_state_changed_data.dtr;
	RTS = event->line_state_changed_data.rts;
	printf("[%ld] dtr: %d, rts: %d\r\n", modMilliseconds(), DTR, RTS);
	checkLineState();
}

void cdc_rx_callback(int itf, cdcacm_event_t *event) {
    portBASE_TYPE xTaskWoken = 0;
	size_t space, read;
	int i;

	space = fifo_remain(&rx_fifo);
	esp_err_t ret = tinyusb_cdcacm_read(itf, usb_rx_buf, space, &read);
	if (ESP_OK == ret) {
		for (i=0; i<read; i++)
			fifo_put(&rx_fifo, usb_rx_buf[i]);
	}

	i = 0;

#if mxDebug
	if (0 == usbEvtPending++) {
		xQueueSendToBackFromISR(usbDbgQueue, &i, &xTaskWoken);
		if (xTaskWoken == pdTRUE)
			portYIELD_FROM_ISR();
	}
#endif
}

void setupDebuggerUSB() {
	tinyusb_config_t tusb_cfg = {};
	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
	tinyusb_config_cdcacm_t acm_cfg = {
		.usb_dev = TINYUSB_USBDEV_0,
		.cdc_port = TINYUSB_CDC_ACM_0,
		.rx_unread_buf_sz = 64,
		.callback_rx = &cdc_rx_callback,
		.callback_rx_wanted_char = NULL,
		.callback_line_state_changed = &line_state_callback,
		.callback_line_coding_changed = NULL
	};

	rx_fifo_buffer = c_malloc(1024);
	fifo_init(&rx_fifo, rx_fifo_buffer, 1024);

#if mxDebug
	usbDbgQueue = xQueueCreate(8, sizeof(uint32_t));
	xTaskCreate(debug_task, "debug", 2048, usbDbgQueue, 8, NULL);
#endif

	ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

	uint32_t count;
	for (count = 0; count < 100; count++) {
		if (tud_cdc_connected()) {
			printf("USB CONNECTED!\r\n");
			break;
		}
		modDelayMilliseconds(50);	// give USB time to come up
	}
}

void ESP_put(uint8_t *c, int count) {
	if (!tud_cdc_connected())
		return;
	while (count) {
		uint32_t amt = count > CONFIG_TINYUSB_CDC_RX_BUFSIZE ? CONFIG_TINYUSB_CDC_RX_BUFSIZE : count;
		tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, c, amt);
		if (ESP_ERR_TIMEOUT == tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 50)) {
			printf("write_flush timeout\n");
		}
		c += amt;
		count -= amt;
	}
}

void ESP_putc(int c) {
	uint8_t ch = c;
	ESP_put(&ch, 1);
}

int ESP_getc(void) {
	uint8_t c;

	if (0 == fifo_get(&rx_fifo, &c))
		return c;
	return -1;
}

uint8_t ESP_setBaud(int baud) {
	return 0;
}

#else

WEAK void ESP_put(uint8_t *c, int count) {
#if (USE_USB == 2)
	int sent = 0;
	while (count > 0) {
		sent = usb_serial_jtag_write_bytes(c, count, 10);
		c += sent;
		count -= sent;
	}   
#else
	uart_write_bytes(USE_UART, (char *)c, count);
#endif
}

WEAK void ESP_putc(int c) {
	char cx = c;
#if (USE_USB == 2)
    usb_serial_jtag_write_bytes(&cx, 1, 1);
#else
	uart_write_bytes(USE_UART, &cx, 1);
#endif
}

WEAK int ESP_getc(void) {
	int amt;
	uint8_t c;
#if (USE_USB == 2)
	amt = usb_serial_jtag_read_bytes(&c, 1, 1);
#else
	amt = uart_read_bytes(USE_UART, &c, 1, 0);
#endif
	return (1 == amt) ? c : -1;
}

WEAK uint8_t ESP_isReadable() {
#if (USE_USB == 2)
	return true;
#else
	size_t s;
	uart_get_buffered_data_len(USE_UART, &s);
	return s > 0;
#endif
}

WEAK uint8_t ESP_setBaud(int baud) {
#if (USE_USB == 2)
	return 1;
#else
	uart_wait_tx_done(USE_UART, 5 * 1000);
	return ESP_OK == uart_set_baudrate(USE_UART, baud);
#endif
}
#endif

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

#if (USE_USB == 1)
	printf("USE TinyUSB\r\n");
	setupDebuggerUSB();
#elif (USE_USB == 2)
#ifdef mxDebug
    xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), 0, 8, NULL);
    printf("START USB CONSOLE!!!\n");
#endif
#else // !USE_USB

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

#ifdef mxDebug
	QueueHandle_t uartQueue;
	uart_driver_install(USE_UART, UART_FIFO_LEN * 2, 0, 8, &uartQueue, 0);
#else
	uart_driver_install(USE_UART, UART_FIFO_LEN * 2, 0, 0, NULL, 0);
#endif

	err = uart_param_config(USE_UART, &uartConfig);
	if (err)
		printf("uart_param_config err %d\r\n", err);
	err = uart_set_pin(USE_UART, USE_UART_TX, USE_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err)
		printf("uart_set_pin err %d\r\n", err);

#ifdef mxDebug
	xTaskCreate(debug_task, "debug", (768 + XT_STACK_EXTRA) / sizeof(StackType_t), uartQueue, 8, NULL);
#endif

#endif	// ! USE_USB

	xTaskCreate(loop_task, "main", kStack, NULL, 4, NULL);
//	xTaskCreatePinnedToCore(loop_task, "main", kStack, NULL, 4, NULL, 0);
}
