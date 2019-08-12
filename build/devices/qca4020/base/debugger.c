/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

#include "qapi_status.h"
#include "qapi_types.h"
#include "qapi_uart.h"

#include "qurt_error.h"
#include "qurt_signal.h"
#include "qurt_thread.h"
#include "qurt_mutex.h"
#include "qurt_timer.h"

qurt_mutex_t gDebugMutex;
static int debuggerSetup = 0;

#if mxDebug

#include "xsmain.h"

#define DEBUG_THREAD_PRIO		(20)
#define DEBUG_THREAD_STACK_SIZE	2048

#define RCV_BUFFER_SIZE				(1024)
#define RCV_BUFFER_COUNT 			(2)

#ifndef MODDEF_DEBUGGER_PORT
	#define MODDEF_DEBUGGER_PORT	QAPI_UART_DEBUG_PORT_E
	//#define MODDEF_DEBUGGER_PORT	QAPI_UART_HS_PORT_E
#endif
#ifndef MODDEF_DEBUGGER_BAUD
	#define MODDEF_DEBUGGER_BAUD	115200
#endif

#define DEBUGGER_EVENT_MASK_RX1		(0x01)
#define DEBUGGER_EVENT_MASK_RX2		(0x02)
#define DEBUGGER_EVENT_MASK_TX		(0x04)

#define CIRC_BUF_SIZE 256 // (RCV_BUFFER_SIZE * RCV_BUFFER_COUNT)

typedef struct circBuf {
	uint8_t 	data[CIRC_BUF_SIZE];
	uint16_t	readIdx;
	uint16_t	writeIdx;
	volatile uint32_t	pendingBytes;
	qbool_t		overflow;
} circBuf;

struct circBuf gRxBuf = { {0}, 0, 0, 0, false };
struct circBuf gTxBuf = { {0}, 0, 0, 0, false };

typedef struct debuggerStruct_s {
	qapi_UART_Handle_t	uart;
	qurt_thread_t		thread;
	char				rxBuffer[RCV_BUFFER_COUNT][RCV_BUFFER_SIZE];
	char				rxQueued;
	qurt_signal_t		event;
	uint32_t rxCalled;
	uint32_t rxReceivedBytes;
	uint32_t txCalled;
	uint32_t txFail;
	uint32_t txBytesSent;
	uint32_t txReallySent;
} debuggerStruct_t;

int gPendingTx = 0;

static debuggerStruct_t	debugger;

static void debuggerRxCB(uint32_t size, void *ref);
static void debuggerTxCB(uint32_t size, void *ref);
int debugger_write_a(const char *buf, int len);
void setupDebugger();

//---------
static void debug_serial_setup()
{
	qapi_UART_Open_Config_t config;
	int ret;

	c_memset(&debugger, 0, sizeof(debugger));

	config.baud_Rate = MODDEF_DEBUGGER_BAUD;
	config.parity_Mode = QAPI_UART_NO_PARITY_E;
	config.num_Stop_Bits = QAPI_UART_1_0_STOP_BITS_E;
	config.bits_Per_Char = QAPI_UART_8_BITS_PER_CHAR_E;
	config.enable_Loopback = false;
	config.enable_Flow_Ctrl = false;
	config.tx_CB_ISR = debuggerTxCB;
	config.rx_CB_ISR = debuggerRxCB;

	qurt_signal_create(&(debugger.event));
	debugger.rxQueued = 0;

	if (QAPI_OK == (ret = qapi_UART_Open(&(debugger.uart), MODDEF_DEBUGGER_PORT, &config))) {
		qapi_UART_Receive(debugger.uart, (char*)(debugger.rxBuffer[0]), RCV_BUFFER_SIZE, (void*)0);
		qapi_UART_Receive(debugger.uart, (char*)(debugger.rxBuffer[1]), RCV_BUFFER_SIZE, (void*)1);
		debugger.rxQueued = 3;	// bits 0 and 1
	}

}

static void debug_task(void *pvParameter)
{
    int         ret;
    uint32_t    attr;
    uint32_t    triggerSignals, curSignals;
    qurt_time_t qtime;

	debug_serial_setup();

	qurt_signal_set(&gMainSignal, kSIG_THREAD_CREATED);

	while (true) {
		qtime = qurt_timer_convert_time_to_ticks(10000, QURT_TIME_MSEC);
		attr = QURT_SIGNAL_ATTR_WAIT_ANY;
		triggerSignals = DEBUGGER_EVENT_MASK_RX1 | DEBUGGER_EVENT_MASK_RX2 | DEBUGGER_EVENT_MASK_TX;

		ret = qurt_signal_wait_timed(&(debugger.event), triggerSignals, attr, &curSignals, qtime);

		if (DEBUGGER_EVENT_MASK_TX & curSignals) {
			qurt_signal_clear(&(debugger.event), DEBUGGER_EVENT_MASK_TX);
			if (!gPendingTx && gTxBuf.pendingBytes) {
				int amt;
				char *buf;

				modCriticalSectionBegin();
				if (gTxBuf.readIdx + gTxBuf.pendingBytes > CIRC_BUF_SIZE)
					amt = CIRC_BUF_SIZE - gTxBuf.readIdx;
				else
					amt = gTxBuf.pendingBytes;

				debugger.txCalled += 1;
				buf = (char*)&(gTxBuf.data[gTxBuf.readIdx]);
				modCriticalSectionEnd();

				ret = qapi_UART_Transmit(debugger.uart, buf, amt, NULL);

				if (!ret) {
					gPendingTx = amt;
				}
				else {
					debugger.txFail += 1;
				}

			}
		}

		if (DEBUGGER_EVENT_MASK_RX1 & curSignals) {
			qurt_signal_clear(&(debugger.event), DEBUGGER_EVENT_MASK_RX1);

			if (!(debugger.rxQueued & 1)) {
				ret = qapi_UART_Receive(debugger.uart, (char*)(debugger.rxBuffer[0]), RCV_BUFFER_SIZE, (void*)0);
				debugger.rxQueued |= 1;
			}
		}
		if (DEBUGGER_EVENT_MASK_RX2 & curSignals) {
			qurt_signal_clear(&(debugger.event), DEBUGGER_EVENT_MASK_RX2);

			if (!(debugger.rxQueued & 2)) {
				ret = qapi_UART_Receive(debugger.uart, (char*)(debugger.rxBuffer[1]), RCV_BUFFER_SIZE, (void*)1);
				debugger.rxQueued |= 2;
			}
		}

		// fxReceiveLoop has Watchdog reset
		if (gRxBuf.pendingBytes)
			qurt_signal_set(&gMainSignal, kSIG_SERVICE_DEBUGGER);	
	}
}


int debugger_write_a(const char *buf, int len) {
	int amt = len;

	while (0 != qurt_mutex_try_lock(&gDebugMutex)) {
			// need the debug mutex
	}
	modCriticalSectionBegin();
	while (amt) {
		if (gTxBuf.pendingBytes >= CIRC_BUF_SIZE)
			break;
		gTxBuf.data[gTxBuf.writeIdx] = *buf++;
		gTxBuf.writeIdx = (gTxBuf.writeIdx + 1) % CIRC_BUF_SIZE;
		gTxBuf.pendingBytes++;
		amt -= 1;
	}
	modCriticalSectionEnd();
	if (!gPendingTx)
		qurt_signal_set(&(debugger.event), DEBUGGER_EVENT_MASK_TX);
	qurt_mutex_unlock(&gDebugMutex);
	return len - amt;
}

void debugger_write(const char *buf, int len) {
	while (len) {
		int amt;
		amt = debugger_write_a(buf, len);
		if (amt) {
			len -= amt;
			buf += amt;
		}
		else {
			modDelayMilliseconds(1);
		}
	}
}

void modLog_transmit(const char *msg)
{
	uint8_t c;

	if (gThe) {
		while (0 != qurt_mutex_try_lock(&gDebugMutex)) {
			// need the debug mutex
		}
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
		qurt_mutex_unlock(&gDebugMutex);	
	}
	else
	{
		debugger_write(msg, c_strlen(msg));
		debugger_write("\r\n", 2);
	}
}

void ESP_putc(int c) {
	char ch = c;
	debugger_write(&ch, 1);
}

int ESP_getc(void) {
	int ch;
	
	if (gRxBuf.pendingBytes < 1)
		return -1;

	modCriticalSectionBegin();
	ch = gRxBuf.data[gRxBuf.readIdx];
	gRxBuf.readIdx = (gRxBuf.readIdx + 1) % CIRC_BUF_SIZE;

	gRxBuf.pendingBytes--;
	modCriticalSectionEnd();

	return ch;
}

uint8_t ESP_isReadable() {
	if (gRxBuf.pendingBytes < 1)
		return 0;
	return 1;
}

static void debuggerRxCB(uint32_t size, void *ref) {
	uint32_t idx = (uint32_t)ref;
	int i = 0;

	debugger.rxCalled++;
	debugger.rxReceivedBytes += size;

	modCriticalSectionBegin();
	while (size--) {
		gRxBuf.data[gRxBuf.writeIdx] = debugger.rxBuffer[idx][i++];
		gRxBuf.writeIdx = (gRxBuf.writeIdx + 1) % CIRC_BUF_SIZE;
		if (gRxBuf.pendingBytes == CIRC_BUF_SIZE)
			gRxBuf.overflow = true;
		else
			gRxBuf.pendingBytes++;
	}
	debugger.rxQueued &= ~(1 << idx);
	modCriticalSectionEnd();
	qurt_signal_set(&(debugger.event), (1 << idx));
}

static void debuggerTxCB(uint32_t size, void *ref) {
	modCriticalSectionBegin();
	debugger.txReallySent += size;
	gPendingTx -= size;
	gTxBuf.readIdx = (gTxBuf.readIdx + size) % CIRC_BUF_SIZE;
	gTxBuf.pendingBytes -= size;
	debugger.txBytesSent += size;
	modCriticalSectionEnd();

	if (gTxBuf.pendingBytes != 0) {
		qurt_signal_set(&(debugger.event), DEBUGGER_EVENT_MASK_TX);
	}
}

#endif

void setupDebugger() {
	if (!debuggerSetup) {
		debuggerSetup = 1;
		qurt_mutex_create(&gDebugMutex);

#if mxDebug
		qurt_thread_attr_t	attr;
		uint8_t		ret;

		qurt_thread_attr_init(&attr);
		qurt_thread_attr_set_name(&attr, "debug");
		qurt_thread_attr_set_priority(&attr, DEBUG_THREAD_PRIO);
		qurt_thread_attr_set_stack_size(&attr, DEBUG_THREAD_STACK_SIZE);
		ret = qurt_thread_create(&debugger.thread, &attr, debug_task, NULL);

		if (QURT_EOK == ret) {
			uint32_t attr;
			attr = QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK;
			if (QURT_EOK != qurt_signal_wait(&gMainSignal, kSIG_THREAD_CREATED, attr)) {
				// wait for it to start
			}
		}
		else
			debugger_write("debug thread failed.", 21);
#endif
	}
}

#if !mxDebug
void modLog_transmit(const char *msg) { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
void debugger_write(const char *buf, int len) { }

#endif
