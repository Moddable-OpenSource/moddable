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
#include "sdk_config.h"

#define USE_USBD_CDC_ACM 0

#if NRF_LOG_ENABLED
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static uint8_t gNRFLogEnabled = 0;

static void NRFLogInit()
{
	ret_code_t err_code;
	err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);
	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void modLog_transmit(const char *msg)
{ 
	static char _msgBuffer[128];
	
	if (0 == gNRFLogEnabled) {
		NRFLogInit();
		gNRFLogEnabled = 1;
	}
	
	uint16_t msgLength = c_strlen(msg) + 1;
	if (msgLength + 3 < sizeof(_msgBuffer)) {
		c_memcpy(_msgBuffer, msg, msgLength);
		NRF_LOG_RAW_INFO("<mod> %s\r\n", _msgBuffer);
	}
}
#endif

#if mxDebug

#if !NRF_LOG_ENABLED

#if !USE_USBD_CDC_ACM

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
#else	// USE_USBD_CDC_ACM

#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#define READ_SIZE 1

static uint8_t m_rx_buffer[READ_SIZE];
static uint8_t m_tx_buffer[NRF_DRV_USBD_EPSIZE];

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);
static void usbd_user_ev_handler(app_usbd_event_type_t event);
static void debug_task(void *pvParameter);

int ESP_getc(void);
void ESP_putc(int c);
void setupDebugger();

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

#define DEBUGGER_STACK	768

#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE	4
#define DEBUG_TASK_CREATED		6
#define DEBUG_READABLE			13
#define DEBUG_WRITABLE			14
#define DEBUG_DRV_ERR			15

static uint32_t xx_tx_data_count = 0;
static uint32_t xx_rx_data_count = 0;
static uint32_t xx_tx_drv_err = 0;

static QueueHandle_t gUARTQueue;

void setupDebugger() {
	ret_code_t ret;

	static const app_usbd_config_t usbd_config = {
		.ev_state_proc = usbd_user_ev_handler
	};

	gUARTQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);

	nrf_drv_clock_lfclk_request(NULL);

	while (!nrf_drv_clock_lfclk_is_running())
		;

	app_usbd_serial_num_generate();

	ret = app_usbd_init(&usbd_config);
	APP_ERROR_CHECK(ret);

	app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
	ret = app_usbd_class_append(class_cdc_acm);
	APP_ERROR_CHECK(ret);
	
	app_usbd_enable();

	xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), NULL, 8, NULL);
}

static void debug_task(void *pvParameter) {
	uint32_t msg;

	msg = DEBUG_TASK_CREATED;
	xQueueSend(gUARTQueue, &msg, 0);

	app_usbd_start();

	while (true) {
        while (app_usbd_event_queue_process())
        	;
        	
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

void usbd_user_ev_handler(app_usbd_event_type_t event)
{
	switch (event)
	{
		case APP_USBD_EVT_DRV_SUSPEND:
			break;
		case APP_USBD_EVT_DRV_RESUME:
			break;
		case APP_USBD_EVT_STARTED:
			break;
		case APP_USBD_EVT_STOPPED:
			app_usbd_disable();
			break;
		case APP_USBD_EVT_POWER_DETECTED:
			if (!nrf_drv_usbd_is_enabled())
				app_usbd_enable();
			break;
		case APP_USBD_EVT_POWER_REMOVED:
			app_usbd_stop();
			break;
		case APP_USBD_EVT_POWER_READY:
			app_usbd_start();
			break;
		default:
			break;
	}
}

void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
	ret_code_t ret;
	app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
	uint32_t msg = 0;

	switch (event)
	{
		case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
			// Setup first transfer
			ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, READ_SIZE);
			break;
		case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
			msg = DEBUG_DRV_ERR;
			xx_tx_drv_err++;
			break;
		case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
			msg = DEBUG_WRITABLE;
			xx_tx_data_count++;
			break;
		case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
			msg = DEBUG_READABLE;
			xx_rx_data_count++;
			break;
		default:
			break;
	}
	if (msg)
		xQueueSend(gUARTQueue, &msg, NULL);
}

void modLog_transmit(const char *msg)
{
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

void ESP_putc(int c) {
	ret_code_t ret;
	uint8_t ch;

	ch = c;
	ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, &ch, 1);
	
	// @@ wait for APP_USBD_CDC_ACM_USER_EVT_TX_DONE event?
}

int ESP_getc(void) {
	uint8_t ch;
	size_t available;
	ret_code_t ret;

	available = app_usbd_cdc_acm_bytes_stored(&m_app_cdc_acm);
	if (available > 0) {
		ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, &ch, 1);
		if (NRF_SUCCESS == ret)
			return ch;
	}

	return -1;
}

uint8_t ESP_isReadable() {
	size_t available;

	available = app_usbd_cdc_acm_bytes_stored(&m_app_cdc_acm);
	return (available > 0);
}
#endif

#else	// NRF_LOG_ENABLED

void setupDebugger() { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
#endif

#else	// !mxDebug

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }

#if !NRF_LOG_ENABLED
void modLog_transmit(const char *msg) { }
#endif

#endif

