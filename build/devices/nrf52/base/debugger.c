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
#include "queue.h"

#include "mc.defines.h"

#include "nrf_libuarte_async.h"
#include "app_fifo.h"

#include "ftdi_trace.h"

#if !USE_DEBUGGER_USBD

#ifdef mxDebug

#define DEBUGGER_STACK	2048
#define kDebuggerTaskPriority	1

#ifndef MODDEF_DEBUGGER_RX_PIN
	#define MODDEF_DEBUGGER_RX_PIN	31
#endif
#ifndef MODDEF_DEBUGGER_TX_PIN
	#define MODDEF_DEBUGGER_TX_PIN	30	
#endif
#ifndef MODDEF_DEBUGGER_BAUDRATE
	#define MODDEF_DEBUGGER_BAUDRATE	NRF_UARTE_BAUDRATE_921600
#endif

// name, uarte_idx, timer0_idx, rtc1_idx, timer1_idx, rx_buf_size, rx_buf_cnt
// We use an app_timer for the timer0
// - sdk_config.h has NRF_LIBUARTE_UARTE_ASYNC_WITH_APP_TIMER
#define DBG_UARTE_NAME	gDebuggerUarte
#define DBG_UARTE_IDX	0
#define DBG_TIMER0_IDX	2
#define DBG_RTC1_IDX	0
#define DBG_TIMER1_IDX	NRF_LIBUARTE_PERIPHERAL_NOT_USED
#define DBG_RX_BUF_SIZE	255
#define DBG_RX_BUF_CNT	3

NRF_LIBUARTE_ASYNC_DEFINE(DBG_UARTE_NAME, DBG_UARTE_IDX, DBG_TIMER0_IDX, DBG_RTC1_IDX, DBG_TIMER1_IDX, DBG_RX_BUF_SIZE, DBG_RX_BUF_CNT);

static uint32_t xx_tx_data_count = 0;
static uint32_t xx_rx_data_count = 0;
static uint32_t xx_tx_drv_err = 0;

/* this queue for freertos signaling to debugger thread */
#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE   4
static QueueHandle_t gUARTQueue;

#define DEBUG_TASK_CREATED		6
#define DEBUG_READABLE			13
#define DEBUG_WRITABLE			14
#define DEBUG_TASK_WRITE		15
#define DEBUG_DRV_ERR			16
#define DEBUG_RX_FIFO_OVERFLOW	17

static app_fifo_t m_rx_fifo;
static app_fifo_t m_tx_fifo;
static uint8_t *tx_fifo_buffer;
static uint8_t *rx_fifo_buffer;

#define tx_buffer_size	255
static uint8_t tx_buffer[tx_buffer_size];

#define MODDEF_DEBUGGER_TX_FIFO_SIZE	2048
#define MODDEF_DEBUGGER_RX_FIFO_SIZE	1024

static __INLINE uint32_t fifo_length(app_fifo_t *const fifo)
{
	uint32_t tmp = fifo->read_pos;
	return fifo->write_pos - tmp;
}

uint32_t fillBufFromFifo(app_fifo_t *fifo, uint8_t *buf, uint32_t bufSize) {
	int i=0;
	while (i<bufSize && (NRF_SUCCESS == app_fifo_get(fifo, buf + i)))
		i++;
	return i;
}

// uarte_handler happens in IRQ
static void uarte_handler(void *context, nrf_libuarte_async_evt_t *p_event)
{
	nrf_libuarte_async_t * p_libuarte = (nrf_libuarte_async_t *)context;
	uint32_t i, msg = 0;
	ret_code_t ret;

	switch (p_event->type) {
		case NRF_LIBUARTE_ASYNC_EVT_ERROR:
			msg = DEBUG_DRV_ERR;
			xx_tx_drv_err++;
			break;
		case NRF_LIBUARTE_ASYNC_EVT_RX_DATA:
			msg = DEBUG_READABLE;
			for (i=0; i<p_event->data.rxtx.length; i++) {
				ret = app_fifo_put(&m_rx_fifo, p_event->data.rxtx.p_data[i]);
				if (NRF_SUCCESS != ret)
					msg = DEBUG_RX_FIFO_OVERFLOW;
			}
			nrf_libuarte_async_rx_free(p_libuarte, p_event->data.rxtx.p_data, p_event->data.rxtx.length);
			xx_rx_data_count++;
			break;
		case NRF_LIBUARTE_ASYNC_EVT_TX_DONE:
			i = fillBufFromFifo(&m_tx_fifo, tx_buffer, tx_buffer_size);
			if (i)
				ret = nrf_libuarte_async_tx(p_libuarte, tx_buffer, i);
			else
				msg = DEBUG_WRITABLE;
			xx_tx_data_count++;
			break;
		default:
			break;
	}
	if (msg)
		xQueueSendFromISR(gUARTQueue, &msg, 0);
}

static uint32_t rxoverflow = 0;
static void debug_task(void *pvParameter) {
	nrf_libuarte_async_t * p_libuarte = (nrf_libuarte_async_t *)pvParameter;
	uint32_t	msg;
	uint8_t		writable = 1;
	uint32_t	i;

	nrf_libuarte_async_enable(p_libuarte);

	msg = DEBUG_TASK_CREATED;
	xQueueSend(gUARTQueue, &msg, 0);

	while (true) {
		if (!xQueueReceive(gUARTQueue, (void*)&msg, portMAX_DELAY))
			continue;

		if (DEBUG_WRITABLE == msg) {
			writable = 1;
		}
		else if (DEBUG_RX_FIFO_OVERFLOW == msg) {
			rxoverflow++;
			modLog("rx fifo overflow error\n");
		}

		if (fifo_length(&m_rx_fifo))
			fxReceiveLoop();

		if (writable && fifo_length(&m_tx_fifo)) {
			writable = 0;
			i = fillBufFromFifo(&m_tx_fifo, tx_buffer, tx_buffer_size);
			if (i) {
				ret_code_t ret;
				ret = nrf_libuarte_async_tx(p_libuarte, tx_buffer, i);
			}
		}
	}
}
		
//---------
void setupDebugger() {
	ret_code_t ret;

#if USE_FTDI_TRACE
	ftdiTraceInit();
#endif

	nrf_libuarte_async_config_t nrf_libuarte_async_config = {
		.tx_pin = MODDEF_DEBUGGER_TX_PIN,
		.rx_pin = MODDEF_DEBUGGER_RX_PIN,
		.baudrate = MODDEF_DEBUGGER_BAUDRATE,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.timeout_us = 100,
		.int_prio = 5
	};

	gUARTQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);

	ret = nrf_libuarte_async_init(&gDebuggerUarte, &nrf_libuarte_async_config, uarte_handler, (void*)&gDebuggerUarte);
	if (ret)
		ftdiTrace("init debuggerUarte failed\n");

	tx_fifo_buffer = c_malloc(MODDEF_DEBUGGER_TX_FIFO_SIZE);
	rx_fifo_buffer = c_malloc(MODDEF_DEBUGGER_RX_FIFO_SIZE);

	ret = app_fifo_init(&m_tx_fifo, tx_fifo_buffer, MODDEF_DEBUGGER_TX_FIFO_SIZE);
	ret = app_fifo_init(&m_rx_fifo, rx_fifo_buffer, MODDEF_DEBUGGER_RX_FIFO_SIZE);
	xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), (void*)&gDebuggerUarte, kDebuggerTaskPriority, NULL);

	ftdiTrace("setupDebugger done.");

}

void flushDebugger() {
}

static int oof = 0;
static int waiting = 0;
void ESP_putc(int c) {
	int ret;
	uint8_t ch;
	uint32_t msg;

	ch = c;
	while (fifo_length(&m_tx_fifo) > m_tx_fifo.buf_size_mask) {
		ftdiTraceAndInt("ESP_putc waiting:", waiting);
		waiting++;
		taskYIELD();
	}

	ret = app_fifo_put(&m_tx_fifo, ch);
	if (NRF_SUCCESS != ret)
		oof++;

	msg = DEBUG_TASK_WRITE;
	xQueueSend(gUARTQueue, &msg, 0);
}

int ESP_getc(void) {
	uint8_t ch;

	if (fifo_length(&m_rx_fifo)) {
		if (NRF_SUCCESS == app_fifo_get(&m_rx_fifo, &ch))
			return ch;
	}

	return -1;
}

#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }

#endif

#endif	// !USE_DEBUGGER_USBD 

