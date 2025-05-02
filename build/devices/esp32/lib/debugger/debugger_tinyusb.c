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

#include "xs.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "mc.defines.h"

#if MODDEF_ECMA419_ENABLED
	#include "common/builtinCommon.h"
#endif

#define WEAK __attribute__((weak))

#include "sdkconfig.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

extern void fx_putc(void *refcon, char c);		//@@

#ifdef mxDebug
	extern xsMachine *gThe;		// this is copied in from main
#endif

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
// printf("fifo_put failed\r\n");
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
// printf("fifo_init - bad size: %ld\r\n", size);
		return -2;		// bad size - needs to be base 2
}

	fifo->buf = buf;
	fifo->size_mask = size - 1;
	fifo->read = 0;
	fifo->write = 0;

	return 0;
}


#ifdef mxDebug

static void debug_task(void *pvParameter)
{
	while (true) {
		uint32_t count;
		if (!fifo_length(&rx_fifo)) {
			usbEvtPending = 0;
			xQueueReceive((QueueHandle_t)pvParameter, (void * )&count, portMAX_DELAY);
		}

		fxReceiveLoop();
	}
}
#endif

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
	// printf("[%ld] dtr: %d, rts: %d\r\n", modMilliseconds(), DTR, RTS);
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

void ESP_put(uint8_t *c, int count) {
	if (!tud_cdc_connected())
		return;
	while (count) {
		uint32_t amt = count > CONFIG_TINYUSB_CDC_RX_BUFSIZE ? CONFIG_TINYUSB_CDC_RX_BUFSIZE : count;
		tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, c, amt);
		if (ESP_ERR_TIMEOUT == tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 50)) {
			// printf("write_flush timeout\n");
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

void setupDebugger(void) {
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
			// printf("USB CONNECTED!\r\n");
			break;
		}
		modDelayMilliseconds(50);	// give USB time to come up
	}
}

