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

#include "xs.h"
#include "xsPlatform.h"
#include "queue.h"

#include "mc.defines.h"

#if !USE_DEBUGGER_USBD || USE_FTDI_TRACE

#if !USE_DEBUGGER_USBD && USE_FTDI_TRACE
	#error Use either FTDI_TRACE or serial debugging, not both
#endif

#include "nrfx_uarte.h"
#include "app_fifo.h"

#ifdef mxDebug

#define DEBUGGER_STACK	2048
#define kDebuggerTaskPriority	5		// (tskIDLE_PRIORITY + 1)

#ifndef MODDEF_DEBUGGER_RX_PIN
	#define MODDEF_DEBUGGER_RX_PIN	31
#endif
#ifndef MODDEF_DEBUGGER_TX_PIN
	#define MODDEF_DEBUGGER_TX_PIN	30	
#endif
#ifndef MODDEF_DEBUGGER_BAUDRATE
	#define MODDEF_DEBUGGER_BAUDRATE	NRF_UARTE_BAUDRATE_921600
#endif

#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE	4
static QueueHandle_t gDebuggerQueue;


static app_fifo_t m_rx_fifo;
static uint8_t *rx_fifo_buffer;

#define rx_buffer_size	1		// 4
static uint8_t rx_buffer[rx_buffer_size];

#define MODDEF_DEBUGGER_RX_FIFO_SIZE	512

#define DEBUG_WRITABLE			14
#define DEBUG_READABLE			15
#define DEBUG_RX_FIFO_OVERFLOW	16
#define DEBUG_TX_FIFO_ERROR		17
#define DEBUG_UARTE_ERROR		18
#define DEBUG_RX_FAIL			19

char notifyOutstanding = 0;
char gReceiveInProgress = 0;

const nrfx_uarte_t gDebuggerUARTE = NRFX_UARTE_INSTANCE(0);

void die(char *x) {
	uint8_t DIE_BUF[256];
	c_strcpy(DIE_BUF, x);
	while (1) {
	}
}
#define DIE(x, ...)	die(x)

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

static void uarte_handler(const nrfx_uarte_event_t *p_event, void *p_context)
{
	nrfx_err_t	err;
	uint8_t		data;
	uint32_t	msg = 0;
	int			i;

	switch (p_event->type) {
		case NRFX_UARTE_EVT_RX_DONE:
			msg = DEBUG_READABLE;
			for (i=0; i<p_event->data.rxtx.bytes; i++) {
				err = app_fifo_put(&m_rx_fifo, p_event->data.rxtx.p_data[i]);
				if (NRF_SUCCESS != err)
					msg = DEBUG_RX_FIFO_OVERFLOW;
			}
			err = nrfx_uarte_rx(&gDebuggerUARTE, rx_buffer, rx_buffer_size);
			if (NRF_SUCCESS != err) {
				gReceiveInProgress = 0;
				msg = DEBUG_RX_FAIL;
			}
			break;
		case NRFX_UARTE_EVT_TX_DONE:
			break;
		case NRFX_UARTE_EVT_ERROR:
			msg = DEBUG_UARTE_ERROR;
			break;
	}

	if (msg && !(++notifyOutstanding))
		xQueueSendFromISR(gDebuggerQueue, &msg, 0);
}

static void debug_task(void *pvParameter) {
	nrfx_err_t	err;
	uint8_t		data;
	uint8_t		writable = 1;
	uint32_t	msg;
	uint32_t	i;

	while (true) {
		if (!fifo_length(&m_rx_fifo)
			/* && !fifo_length(&m_tx_fifo) */
			) {
			xQueueReceive(gDebuggerQueue, (void*)&msg, 1000);
			notifyOutstanding = 0;
		}

		if (!gReceiveInProgress) {
			err = nrfx_uarte_rx(&gDebuggerUARTE, rx_buffer, rx_buffer_size);
			if (NRFX_SUCCESS == err)
				gReceiveInProgress = 1;
			else
				DIE("nrfx_uarte_rx fail");
		}

		// necessary to allow xsbug to break a running app
		if (fifo_length(&m_rx_fifo))
			fxReceiveLoop();
	}
}

//---------
#if USE_FTDI_TRACE
void setupSerial() {
#else
void setupDebugger() {
#endif
	ret_code_t err;

	nrfx_uarte_config_t uart_config = {
		.pseltxd = MODDEF_DEBUGGER_TX_PIN,
		.pselrxd = MODDEF_DEBUGGER_RX_PIN,
		.pselcts = NRF_UARTE_PSEL_DISCONNECTED,
		.pselrts = NRF_UARTE_PSEL_DISCONNECTED,
		.p_context = NULL,
		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = MODDEF_DEBUGGER_BAUDRATE,
		.interrupt_priority = 5
	};

	err = nrfx_uarte_init(&gDebuggerUARTE, &uart_config, uarte_handler);
	if (NRFX_SUCCESS != err)
		DIE("nrfx_uarte_init fail");

	rx_fifo_buffer = c_malloc(MODDEF_DEBUGGER_RX_FIFO_SIZE);
	err = app_fifo_init(&m_rx_fifo, rx_fifo_buffer, MODDEF_DEBUGGER_RX_FIFO_SIZE);

	gDebuggerQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);

	xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), (void*)&gDebuggerUARTE, kDebuggerTaskPriority, NULL);
}

#if USE_FTDI_TRACE
void serial_put(uint8_t *buf, uint32_t len) {
	nrfx_uarte_tx(&gDebuggerUARTE, buf, len);
}

#else
void flushDebugger() {
}

void ESP_putc(int c) {
	int ret;
	uint8_t ch;
	uint32_t msg = DEBUG_WRITABLE;

	ch = c;
	while (nrfx_uarte_tx_in_progress(&gDebuggerUARTE))
		;
	ret = nrfx_uarte_tx(&gDebuggerUARTE, &ch, 1);
	if (NRF_SUCCESS != ret)
		DIE("nrfx_uarte_tx failed");

}

int ESP_getc(void) {
	uint8_t ch;
	uint32_t msg = DEBUG_READABLE;

	if (NRF_SUCCESS == app_fifo_get(&m_rx_fifo, &ch))
		return ch;

	if (!notifyOutstanding++)
		xQueueSend(gDebuggerQueue, &msg, 0);

	return -1;
}
#endif	// ! USE_FTDI_TRACE

#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }

#endif

#endif	// !USE_DEBUGGER_USBD 

