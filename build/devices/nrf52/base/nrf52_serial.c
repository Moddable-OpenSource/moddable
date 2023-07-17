/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "queue.h"

#include "mc.defines.h"

#include "nrf_libuarte_async.h"
#include "app_fifo.h"

#if !defined(MODDEF_SERIAL_BAUDRATE) && !defined(MODDEF_SERIAL_TX_PIN) && !defined(MODDEF_SERIAL_RX_PIN)
// need to define at least one of baudrate, tx_pin or rx_pin to enable this
#else

#define SERIAL_TASK_STACK	2048
#define kSerialTaskPriority	_PRIO_APP_LOW_MID	// 1

#ifndef MODDEF_SERIAL_RX_PIN
	#define MODDEF_SERIAL_RX_PIN	31
#endif
#ifndef MODDEF_SERIAL_TX_PIN
	#define MODDEF_SERIAL_TX_PIN	30	
#endif
#ifndef MODDEF_SERIAL_BAUDRATE
	#define MODDEF_SERIAL_BAUDRATE	NRF_UARTE_BAUDRATE_921600
#endif

// name, uarte_idx, timer0_idx, rtc1_idx, timer1_idx, rx_buf_size, rx_buf_cnt
// The timer is an APP_TIMER
// - sdk_config.h has NRF_LIBUARTE_UARTE_ASYNC_WITH_APP_TIMER
#define SER_UARTE_NAME	gSecondUarte
#define SER_UARTE_IDX	0
#define SER_TIMER0_IDX	1
#define SER_RTC1_IDX	0
#define SER_TIMER1_IDX	NRF_LIBUARTE_PERIPHERAL_NOT_USED
#define SER_RX_BUF_SIZE	255
#define SER_RX_BUF_CNT	3

NRF_LIBUARTE_ASYNC_DEFINE(SER_UARTE_NAME, SER_UARTE_IDX, SER_TIMER0_IDX, SER_RTC1_IDX, SER_TIMER1_IDX, SER_RX_BUF_SIZE, SER_RX_BUF_CNT);

static uint32_t ser_tx_data_count = 0;
static uint32_t ser_rx_data_count = 0;
static uint32_t ser_tx_drv_err = 0;

/* this queue for freertos signaling to debugger thread */
#define SERIAL_QUEUE_LEN			8
#define SERIAL_QUEUE_ITEM_SIZE   4
static QueueHandle_t gSerialQueue;

#define SERIAL_TASK_CREATED		6
#define SERIAL_READABLE			13
#define SERIAL_WRITABLE			14
#define SERIAL_TASK_WRITE		15
#define SERIAL_DRV_ERR			16
#define SERIAL_RX_FIFO_OVERFLOW	17

static app_fifo_t ser_rx_fifo;
static app_fifo_t ser_tx_fifo;
static uint8_t *ser_tx_fifo_buffer;
static uint8_t *ser_rx_fifo_buffer;

#define ser_tx_buffer_size	255
static uint8_t ser_tx_buffer[ser_tx_buffer_size];

#define MODDEF_SERIAL_TX_FIFO_SIZE	1024
#define MODDEF_SERIAL_RX_FIFO_SIZE	1024

static __INLINE uint32_t fifo_length(app_fifo_t *const fifo)
{
	uint32_t ret, tmp = fifo->read_pos;
	return fifo->write_pos - tmp;
}

static uint32_t fillBufFromFifo(app_fifo_t *fifo, uint8_t *buf, uint32_t bufSize) {
	int i=0;
	while (i<bufSize && (NRF_SUCCESS == app_fifo_get(fifo, buf + i)))
		i++;
	return i;
}

// uart_handler happens in IRQ
static void serial_handler(void *context, nrf_libuarte_async_evt_t *p_event)
{
	nrf_libuarte_async_t * p_libuarte = (nrf_libuarte_async_t *)context;
	uint32_t i, msg = 0;
	ret_code_t ret;

	switch (p_event->type) {
		case NRF_LIBUARTE_ASYNC_EVT_ERROR:
			msg = SERIAL_DRV_ERR;
			ser_tx_drv_err++;
			break;
		case NRF_LIBUARTE_ASYNC_EVT_RX_DATA:
			msg = SERIAL_READABLE;
			for (i=0; i<p_event->data.rxtx.length; i++) {
				ret = app_fifo_put(&ser_rx_fifo, p_event->data.rxtx.p_data[i]);
				if (NRF_SUCCESS != ret) {
					msg = SERIAL_RX_FIFO_OVERFLOW;
					break;
				}
			}
			nrf_libuarte_async_rx_free(p_libuarte, p_event->data.rxtx.p_data, p_event->data.rxtx.length);
			ser_rx_data_count++;
			break;
		case NRF_LIBUARTE_ASYNC_EVT_TX_DONE:
			i = fillBufFromFifo(&ser_tx_fifo, ser_tx_buffer, ser_tx_buffer_size);
			if (i)
				ret = nrf_libuarte_async_tx(p_libuarte, ser_tx_buffer, i);
			else
				msg = SERIAL_WRITABLE;
			ser_tx_data_count++;
			break;
		default:
			break;
	}
	if (msg)
		xQueueSendFromISR(gSerialQueue, &msg, 0);
}

static uint32_t serialrxoverflow = 0;
static void serial_task(void *pvParameter) {
	nrf_libuarte_async_t * p_libuarte = (nrf_libuarte_async_t *)pvParameter;
	uint32_t	msg;
	uint8_t		writable = 1;
	uint32_t	i;

	nrf_libuarte_async_enable(p_libuarte);

	msg = SERIAL_TASK_CREATED;
	xQueueSend(gSerialQueue, &msg, 0);

	while (true) {
		if (!xQueueReceive(gSerialQueue, (void*)&msg, portMAX_DELAY))
			continue;

		if (SERIAL_WRITABLE == msg) {
			writable = 1;
		}
		else if (SERIAL_RX_FIFO_OVERFLOW == msg) {
			serialrxoverflow++;
			ftdiTrace("rx fifo overflow error\n");
		}

		if (fifo_length(&ser_rx_fifo))
			fxReceiveLoop();

		if (writable && fifo_length(&ser_tx_fifo)) {
			writable = 0;
			i = fillBufFromFifo(&ser_tx_fifo, ser_tx_buffer, ser_tx_buffer_size);
			if (i) {
				ret_code_t ret;
				ret = nrf_libuarte_async_tx(p_libuarte, ser_tx_buffer, i);
			}
		}
	}
}
		
//---------
void setupSerial() {
	ret_code_t ret;

	nrf_libuarte_async_config_t nrf_libuarte_async_config = {
		.tx_pin = MODDEF_SERIAL_TX_PIN,
		.rx_pin = MODDEF_SERIAL_RX_PIN,
		.baudrate = MODDEF_SERIAL_BAUDRATE,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.timeout_us = 100,
		.int_prio = 5
	};

	gSerialQueue = xQueueCreate(SERIAL_QUEUE_LEN, SERIAL_QUEUE_ITEM_SIZE);

	ret = nrf_libuarte_async_init(&gSecondUarte, &nrf_libuarte_async_config, serial_handler, (void*)&gSecondUarte);
	if (ret)
		modLog("init serial Uarte failed\n");

	ser_tx_fifo_buffer = c_malloc(MODDEF_SERIAL_TX_FIFO_SIZE);
	ser_rx_fifo_buffer = c_malloc(MODDEF_SERIAL_RX_FIFO_SIZE);

	ret = app_fifo_init(&ser_tx_fifo, ser_tx_fifo_buffer, MODDEF_SERIAL_TX_FIFO_SIZE);
	ret = app_fifo_init(&ser_rx_fifo, ser_rx_fifo_buffer, MODDEF_SERIAL_RX_FIFO_SIZE);
	xTaskCreate(serial_task, "serial", SERIAL_TASK_STACK/sizeof(StackType_t), (void*)&gSecondUarte, kSerialTaskPriority, NULL);

}

void flushSerial() {
}

static int seroof = 0;
static int serwaiting = 0;
void serial_putc(int c) {
	int ret;
	uint8_t ch;
	uint32_t msg;

	ch = c;
	while (fifo_length(&ser_tx_fifo) > ser_tx_fifo.buf_size_mask) {
		ftdiTraceAndInt("putc waiting", serwaiting);
		serwaiting++;
		taskYIELD();
	}

	ret = app_fifo_put(&ser_tx_fifo, ch);
	if (NRF_SUCCESS != ret)
		seroof++;

	msg = SERIAL_TASK_WRITE;
	xQueueSend(gSerialQueue, &msg, 0);
}

void serial_put(uint8_t *buf, uint32_t len) {
	while (len--)
		serial_putc(*buf++);
}

int serial_getc(void) {
	uint8_t ch;

	if (fifo_length(&ser_rx_fifo)) {
		if (NRF_SUCCESS == app_fifo_get(&ser_rx_fifo, &ch))
			return ch;
	}

	return -1;
}

#endif
