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
#include "nrfx_uart.h"
#include "queue.h"

#if mxDebug

#define DEBUGGER_STACK	768

#ifndef MODDEF_DEBUGGER_RX_PIN
	#define MODDEF_DEBUGGER_RX_PIN	31
#endif
#ifndef MODDEF_DEBUGGER_TX_PIN
	#define MODDEF_DEBUGGER_TX_PIN	30	
#endif
#ifndef MODDEF_DEBUGGER_BAUDRATE
	#define MODDEF_DEBUGGER_BAUDRATE	NRF_UART_BAUDRATE_115200
#endif

nrfx_uart_config_t gDebuggerUartConfig = {
	.pseltxd = MODDEF_DEBUGGER_TX_PIN,
	.pselrxd = MODDEF_DEBUGGER_RX_PIN,
	.pselcts = -1,
	.pselrts = -1,
	.p_context = NULL,
	.hwfc = NRF_UART_HWFC_DISABLED,
	.parity = NRF_UART_PARITY_EXCLUDED,
	.baudrate = MODDEF_DEBUGGER_BAUDRATE,
	.interrupt_priority = UART_DEFAULT_CONFIG_IRQ_PRIORITY };
		
nrfx_uart_t gDebuggerUart = {
    .p_reg        = NRFX_CONCAT_2(NRF_UART, 0),
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_UART, 0, _INST_IDX),
};

int ESP_getc(void);
void ESP_putc(int c);
void setupDebugger();

static uint32_t xx_tx_data_count = 0;
static uint32_t xx_rx_data_count = 0;
static uint32_t xx_tx_drv_err = 0;

#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE           4
static QueueHandle_t gUARTQueue;

#define DEBUG_TASK_CREATED    6
#define DEBUG_READABLE        13
#define DEBUG_WRITABLE        14
#define DEBUG_DRV_ERR         15

// uart_handler happens in IRQ
static void uart_handler(nrfx_uart_event_t const *p_event, void *p_context) {
	uint32_t msg = 0;

	switch (p_event->type) {
		case NRFX_UART_EVT_TX_DONE:
			msg = DEBUG_WRITABLE;
			xx_tx_data_count++;
			break;
		case NRFX_UART_EVT_RX_DONE:
			msg = DEBUG_READABLE;
			xx_rx_data_count++;
			break;
		case NRFX_UART_EVT_ERROR:
			msg = DEBUG_DRV_ERR;
			xx_tx_drv_err++;
			break;
	}
	if (msg)
		xQueueSendFromISR(gUARTQueue, &msg, NULL);
}

static void debug_task(void *pvParameter) {
	uint32_t	msg;

	msg = DEBUG_TASK_CREATED;
	xQueueSend(gUARTQueue, &msg, 0);

	nrfx_uart_rx_enable(&gDebuggerUart);

	while (true) {
		if (!xQueueReceive(gUARTQueue, (void*)&msg, portMAX_DELAY))
			continue;

		if (DEBUG_READABLE == msg) {
			fxReceiveLoop();		
		}
		else if (DEBUG_DRV_ERR == msg) {
			modLog("uart error\n");
		}
	}
}
		
//---------
void setupDebugger() {
	ret_code_t ret;

	gUARTQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);

	ret = nrfx_uart_init(&gDebuggerUart, &gDebuggerUartConfig, uart_handler);

	xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), NULL, 8, NULL);
}

void modLog_transmit(const char *msg)
{
	int ret;
	uint8_t c;

#if mxDebug
	if (gThe) {
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
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
	while (nrfx_uart_tx_in_progress(&gDebuggerUart))
		taskYIELD();

	while (NRF_ERROR_BUSY == (ret = nrfx_uart_tx(&gDebuggerUart, &ch, 1)))
		taskYIELD();

	if (NRF_SUCCESS != ret)
		oof++;
}

static int debugRead = 0;
int ESP_getc(void) {
	uint8_t ch;
	int ret;

	if (nrfx_uart_rx_ready(&gDebuggerUart)) {
		while (NRF_ERROR_BUSY == (ret = nrfx_uart_rx(&gDebuggerUart, &ch, 1)))
			taskYIELD();

		if (NRF_SUCCESS == ret)
			return ch;
	}

	return -1;
}

uint8_t ESP_isReadable() {
	uint8_t ret;
	ret = nrfx_uart_rx_ready(&gDebuggerUart);
	return ret;
}
#else
void modLog_transmit(const char *msg) { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
#endif

