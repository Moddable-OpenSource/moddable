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

#include "mc.defines.h"

#include "nrf.h"
#include "nrf_serial.h"

#include "queue.h"

#if mxDebug

#define DEBUGGER_STACK	768
#define SERIAL_BUFFER_SIZE			(1024)

#ifndef MODDEF_DEBUGGER_RX_PIN
	#define MODDEF_DEBUGGER_RX_PIN	31
#endif
#ifndef MODDEF_DEBUGGER_TX_PIN
	#define MODDEF_DEBUGGER_TX_PIN	30	
#endif
#ifndef MODDEF_DEBUGGER_BAUDRATE
	#define MODDEF_DEBUGGER_BAUDRATE	NRF_UART_BAUDRATE_115200
#endif

static void sleep_handler(void);
static void event_handler(struct nrf_serial_s const *p_serial, nrf_serial_event_t event);

NRF_SERIAL_DRV_UART_CONFIG_DEF(gDebuggerUartConfig,
		MODDEF_DEBUGGER_RX_PIN, MODDEF_DEBUGGER_TX_PIN,
		-1, -1, NRF_UART_HWFC_DISABLED, NRF_UART_PARITY_EXCLUDED,
		MODDEF_DEBUGGER_BAUDRATE, UART_DEFAULT_CONFIG_IRQ_PRIORITY);
		
NRF_SERIAL_QUEUES_DEF(gDebuggerQueues, SERIAL_BUFFER_SIZE, SERIAL_BUFFER_SIZE);

#define SERIAL_BUFF_TX_SIZE 1     // SERIAL_BUFFER_SIZE
#define SERIAL_BUFF_RX_SIZE 1     // SERIAL_BUFFER_SIZE
NRF_SERIAL_BUFFERS_DEF(gDebuggerBuffers, SERIAL_BUFF_TX_SIZE, SERIAL_BUFF_RX_SIZE);

NRF_SERIAL_CONFIG_DEF(gDebuggerSerialConfig, NRF_SERIAL_MODE_DMA, &gDebuggerQueues, &gDebuggerBuffers, event_handler, sleep_handler);

NRF_SERIAL_UART_DEF(gDebuggerUarte, 0);

int ESP_getc(void);
void ESP_putc(int c);
void setupDebugger();

static uint8_t readable = 0;
static volatile uint8_t sReceivedRx = 1;

static void sleep_handler(void) {
	__WFE();
	__SEV();
	__WFE();
}

static uint32_t xx_tx_done_count = 0;
static uint32_t xx_rx_data_count = 0;

#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE	4
static QueueHandle_t gUARTQueue;

#define DEBUG_TASK_CREATED	6
#define DEBUG_READABLE		13

// event_handler happens in IRQ
static void event_handler(struct nrf_serial_s const *p_serial, nrf_serial_event_t event) {
	uint32_t msg;
	switch (event) {
		case NRF_SERIAL_EVENT_TX_DONE:
                        xx_tx_done_count++;
			break;
		case NRF_SERIAL_EVENT_RX_DATA:
			readable = 1;
            xx_rx_data_count++;
			msg = DEBUG_READABLE;
			xQueueSendFromISR(gUARTQueue, &msg, NULL);
			break;
		case NRF_SERIAL_EVENT_DRV_ERR:
			break;
		case NRF_SERIAL_EVENT_FIFO_ERR:
			break;
	}
}

static void debug_task(void *pvParameter) {
	uint32_t	msg;

	msg = DEBUG_TASK_CREATED;
	xQueueSend(gUARTQueue, &msg, 0);

	while (true) {
		if (!xQueueReceive(gUARTQueue, (void*)&msg, portMAX_DELAY))
			continue;

		if (DEBUG_READABLE == msg) {
			sReceivedRx = 0;
			fxReceiveLoop();		
		}
	}
}
		
//---------
void setupDebugger() {
	uint32_t	msg = 0;
	ret_code_t ret;

	ret = nrf_serial_init(&gDebuggerUarte, &gDebuggerUartConfig, &gDebuggerSerialConfig);

	gUARTQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);

	xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), NULL, 8, NULL);
}

void modLog_transmit(const char *msg)
{
	int ret;
	uint8_t c;

#if mxDebug
	if (gThe) {
//		while (0 != (c = c_read8(msg++)))
//			fx_putc(gThe, c);
//		fx_putc(gThe, 0);
		while (NRF_ERROR_BUSY == (ret = nrf_serial_write(&gDebuggerUarte, msg, c_strlen(msg)+1, NULL, NRF_SERIAL_MAX_TIMEOUT)))
			nrf52_delay_us(10);
	}
	else
#endif
	while (0 != (c = c_read8(msg++)))
		ESP_putc(c);
	ESP_putc('\r');
	ESP_putc('\n');
}

static int oof = 0;
void ESP_putc(int c) {
	int ret;
	uint8_t ch;

	ch = c;
	while (NRF_ERROR_BUSY == (ret = nrf_serial_write(&gDebuggerUarte, &ch, 1, NULL, NRF_SERIAL_MAX_TIMEOUT)))
		nrf52_delay_us(10);

        if (NRF_SUCCESS != ret)
          oof++;
}

static int debugRead = 0;
int ESP_getc(void) {
	uint8_t ch;
	int ret;

	while (NRF_ERROR_BUSY == (ret = nrf_serial_read(&gDebuggerUarte, &ch, 1, NULL, 0)))
		nrf52_delay_us(10);

	if (NRF_SUCCESS == ret)
		return ch;

	return -1;
}

uint8_t ESP_isReadable() { return 0; }
uint8_t xESP_isReadable() {
	uint8_t ret;
	ret = nrf_drv_uart_rx_ready(&gDebuggerUarte);
	return ret;
}
#else
void modLog_transmit(const char *msg) { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
#endif

