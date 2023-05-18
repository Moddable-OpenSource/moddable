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

#if USE_DEBUGGER_USBD

//#ifdef mxDebug

#include "nrf.h"
#include "nrf_queue.h"
#include "nrf_drv_usbd.h"
#include "nrf_ringbuf.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "sdk_config.h"
#include "app_usbd_vendor.h"

#include "task.h"
#include "queue.h"
#include "ftdi_trace.h"

#include "semphr.h"

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);
static void usbd_user_ev_handler(app_usbd_event_type_t event);
static void usbd_task(void *pvParameter);
static void usb_new_event_isr_handler(app_usbd_internal_evt_t const * const p_event, bool queued);

static void vendor_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_vendor_user_event_t event);

#define CDC_ACM_COMM_INTERFACE	0
#define CDC_ACM_COMM_EPIN		NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE	1
#define CDC_ACM_DATA_EPIN		NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT		NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(
	m_app_cdc_acm,
	cdc_acm_user_ev_handler,
	CDC_ACM_COMM_INTERFACE,
	CDC_ACM_DATA_INTERFACE,
	CDC_ACM_COMM_EPIN,
	CDC_ACM_DATA_EPIN,
	CDC_ACM_DATA_EPOUT,
	APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);


// Vendor port is to act similar to cdc/acm
#define VENDOR_COMM_INTERFACE	2
#define VENDOR_COMM_EPIN		NRF_DRV_USBD_EPIN3
#define VENDOR_DATA_INTERFACE 	3
#define VENDOR_DATA_EPIN		NRF_DRV_USBD_EPIN4
#define VENDOR_DATA_EPOUT		NRF_DRV_USBD_EPOUT4

APP_USBD_VENDOR_GLOBAL_DEF(
	m_app_vendor,
	vendor_user_ev_handler,
	VENDOR_COMM_INTERFACE,
	VENDOR_DATA_INTERFACE,
	VENDOR_COMM_EPIN,
	VENDOR_DATA_EPIN,
	VENDOR_DATA_EPOUT,
	APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);
		

#define USBD_STACK_SIZE			256
#define USBD_PRIORITY			5			// 1
#define USB_THREAD_MAX_BLOCK_TIME portMAX_DELAY

#define kTXBufferSize 2048
// #define kTXBufferSize 512

typedef struct txBufferRecord txBufferRecord;
typedef txBufferRecord *txBuffer;

struct txBufferRecord {
	struct txBufferRecord *next;

	uint16_t	size;
	uint8_t		buffer[1];
};

static txBuffer gTxBufferList = NULL;
static uint8_t m_tx_buffer[kTXBufferSize];
static int16_t m_tx_buffer_index = 0;

static char m_rx_buffer[1];
NRF_QUEUE_DEF(uint8_t, m_rx_queue, 4096, NRF_QUEUE_MODE_OVERFLOW);

static volatile uint8_t m_usb_connected = false;

static txBuffer gVendorTxBufferList = NULL;
static uint8_t m_vendor_tx_buffer[kTXBufferSize];
static int16_t m_vendor_tx_buffer_index = 0;

static char m_vendor_rx_buffer[1];
NRF_QUEUE_DEF(uint8_t, m_vendor_rx_queue, 2048, NRF_QUEUE_MODE_OVERFLOW);

static uint8_t m_vendor_usb_connected = false;
static uint8_t m_vendor_usb_restarted = false;
static uint8_t m_vendor_usb_reopened = false;
static uint8_t m_vendor_usb_closed = false;

static TaskHandle_t m_usbd_thread;

#define usbMutexTake() xSemaphoreTake(gUSBMutex, portMAX_DELAY)
#define usbMutexGive() xSemaphoreGive(gUSBMutex)
static SemaphoreHandle_t gUSBMutex = NULL;

static void usb_debug_task(void *pvParameter);
#define DEBUG_TASK_PRIORITY		2
static QueueHandle_t uartQueue;

void setupDebugger(void)
{
	uint32_t count;

#if USE_FTDI_TRACE
    ftdiTraceInit();
#endif

	if (!gUSBMutex)
		gUSBMutex = xSemaphoreCreateMutex();

	uartQueue = xQueueCreate(5, sizeof(uint32_t));
	xTaskCreate(usb_debug_task, "debug", configMINIMAL_STACK_SIZE, uartQueue, 4, NULL);

	app_usbd_serial_num_generate();

	if (pdPASS != xTaskCreate(usbd_task, "USBD", USBD_STACK_SIZE, NULL, USBD_PRIORITY, &m_usbd_thread))
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
        
    ftdiTrace("Waiting for USBD port connection");
	
	// Wait up to 7000 ms for host serial port initialization and connection
	for (count = 0; count < 700; count++) {
		if (m_usb_connected)
			break;

		modDelayMilliseconds(10);
	}
}

void flushDebugger()
{
	uint32_t count = 0;
	
	while ((NULL != gTxBufferList) && (count++ < 10)) {
		modDelayMilliseconds(100);
	}
}

static void usb_debug_task(void *pvParameter)
{
	while (true) {
		uint32_t event;

		if (!xQueueReceive((QueueHandle_t)pvParameter, (void *)&event, portMAX_DELAY))
			continue;
		if (1 == event) {
			fxReceiveLoop();
		}
	}
}

void usb_new_event_isr_handler(app_usbd_internal_evt_t const * const p_event, bool queued)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UNUSED_PARAMETER(p_event);
    UNUSED_PARAMETER(queued);
    ASSERT(m_usbd_thread != NULL);

    vTaskNotifyGiveFromISR(m_usbd_thread, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void usbd_task(void *pvParameter)
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
		.ev_isr_handler = usb_new_event_isr_handler,
		.ev_state_proc  = usbd_user_ev_handler
    };
    UNUSED_PARAMETER(pvParameter);

    ret = app_usbd_init(&usbd_config);
    if (0 == ret) {
    	app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    	ret = app_usbd_class_append(class_cdc_acm);

		app_usbd_class_inst_t const * class_vendor = app_usbd_vendor_class_inst_get(&m_app_vendor);
		ret = app_usbd_class_append(class_vendor);
    }
    if (0 == ret) {
		ret = app_usbd_power_events_enable();
    }
	if (0 == ret) {
		app_usbd_enable();
	}
	if (0 != ret) {
    	ftdiTrace("usbd task init failed");
		return;
	}
	
    // Set the first event to make sure that USB queue is processed after it is started
    UNUSED_RETURN_VALUE(xTaskNotifyGive(xTaskGetCurrentTaskHandle()));

    // Enter main loop.
    for (;;)
    {
        /* Waiting for event */
        UNUSED_RETURN_VALUE(ulTaskNotifyTake(pdTRUE, USB_THREAD_MAX_BLOCK_TIME));
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
    }
}

static app_usbd_cdc_acm_ctx_t * cdc_acm_ctx_get(app_usbd_cdc_acm_t const * p_cdc_acm)
{
    ASSERT(p_cdc_acm != NULL);
    ASSERT(p_cdc_acm->specific.p_data != NULL);
    return &p_cdc_acm->specific.p_data->ctx;
}

static app_usbd_vendor_ctx_t *vendor_ctx_get(app_usbd_vendor_t const *p_vendor)
{
    ASSERT(p_vendor != NULL);
    ASSERT(p_vendor->specific.p_data != NULL);
    return &p_vendor->specific.p_data->ctx;
}


void usbd_user_ev_handler(app_usbd_event_type_t event)
{
	switch(event) {
		case APP_USBD_EVT_STOPPED:
//    		ftdiTrace("APP_USBD_EVT_STOPPED");
			app_usbd_disable();
			break;

		case APP_USBD_EVT_STARTED:
//    		ftdiTrace("APP_USBD_EVT_STARTED");
			break;

		case APP_USBD_EVT_POWER_DETECTED:
//    		ftdiTrace("APP_USBD_EVT_POWER_DETECTED");
			if (!nrf_drv_usbd_is_enabled()) {
//    			ftdiTrace("calling app_usbd_enable");
				app_usbd_enable();
			}
//			else
//    			ftdiTrace("usbd already enabled");
			break;

		case APP_USBD_EVT_POWER_REMOVED:
//    		ftdiTrace("APP_USBD_EVT_POWER_REMOVED");
			app_usbd_stop();
			break;

		case APP_USBD_EVT_POWER_READY:
//    		ftdiTrace("APP_USBD_EVT_POWER_READY");
			if (!nrf_drv_usbd_is_enabled()) {
//    			ftdiTrace("calling app_usbd_enable");
				app_usbd_enable();
			}
//			else
//    			ftdiTrace("usbd already enabled");
			app_usbd_start();
			break;

		case APP_USBD_EVT_DRV_SUSPEND:
    		ftdiTrace("APP_USBD_EVT_DRV_SUSPEND");
			break;

		case APP_USBD_EVT_DRV_RESUME:
    		ftdiTrace("APP_USBD_EVT_DRV_RESUME");
			break;

		case APP_USBD_EVT_STATE_CHANGED:
			break;

		case APP_USBD_EVT_DRV_RESET:
//    		ftdiTrace("APP_USBD_EVT_DRV_RESET");
			break;

		default:
    		ftdiTrace("APP_USBD_EVT_    unknown");
			break;
	}
}

// Helper functions required to send ZLP packet at end of every TX message
#define APP_USBD_CDC_ACM_DATA_IFACE_IDX 1    /**< CDC ACM class data interface index. */
#define APP_USBD_CDC_ACM_DATA_EPIN_IDX  0    /**< CDC ACM data class endpoint IN index. */

static nrf_drv_usbd_ep_t data_ep_in_addr_get(app_usbd_class_inst_t const * p_inst)
{
    app_usbd_class_iface_conf_t const * class_iface;
    class_iface = app_usbd_class_iface_get(p_inst, APP_USBD_CDC_ACM_DATA_IFACE_IDX);

    app_usbd_class_ep_conf_t const * ep_cfg;
    ep_cfg = app_usbd_class_iface_ep_get(class_iface, APP_USBD_CDC_ACM_DATA_EPIN_IDX);

    return app_usbd_class_ep_address_get(ep_cfg);
}

// Send the next queued buffer with ZLP packet
static ret_code_t sendNextBuffer()
{
	ret_code_t ret;
	txBuffer buffer = gTxBufferList;

	if (NULL == buffer)
		return 0;

    app_usbd_class_inst_t const * p_inst = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    app_usbd_cdc_acm_ctx_t * p_cdc_acm_ctx = cdc_acm_ctx_get(&m_app_cdc_acm);

    bool dtr_state = (p_cdc_acm_ctx->line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR) ? true : false;
    if (!dtr_state) {
		ftdiTrace("ACM - DTR line not set! port not opened");
        return NRF_ERROR_INVALID_STATE;	// port not opened
    }
    
    nrf_drv_usbd_ep_t ep = data_ep_in_addr_get(p_inst);
    
	NRF_DRV_USBD_TRANSFER_IN_ZLP(transfer, &buffer->buffer[0], buffer->size);
//	ftdiTraceAndInt("ACM - sending buffer, size=", buffer->size);
//	ftdiTraceHex(&buffer->buffer[0], buffer->size);
	return app_usbd_ep_transfer(ep, &transfer);
}

static ret_code_t sendNextVendorBuffer()
{
	ret_code_t ret;
	txBuffer buffer = gVendorTxBufferList;

	if (NULL == buffer)
		return 0;

    app_usbd_class_inst_t const * p_inst = app_usbd_vendor_class_inst_get(&m_app_vendor);
    app_usbd_vendor_ctx_t * p_vendor_ctx = vendor_ctx_get(&m_app_vendor);

    bool dtr_state = (p_vendor_ctx->line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR) ? true : false;
    if (!dtr_state) {
		ftdiTrace("vendor - DTR line not set! port not opened");
        return NRF_ERROR_INVALID_STATE;	// port not opened
    }
    
    nrf_drv_usbd_ep_t ep = data_ep_in_addr_get(p_inst);
    
	NRF_DRV_USBD_TRANSFER_IN_ZLP(transfer, &buffer->buffer[0], buffer->size);
	ftdiTraceAndInt("vendor - sending buffer, size=", buffer->size);
	ftdiTraceHex(&buffer->buffer[0], buffer->size);
	return app_usbd_ep_transfer(ep, &transfer);
}

void sendAVendorMsg(uint8_t *msg, int len) {
	if (!m_vendor_usb_connected)
		return;

	txBuffer buffer = c_calloc(sizeof(txBufferRecord) + len, 1);
	if (NULL == buffer) return;

	buffer->size = len;
	c_memmove(&buffer->buffer[0], msg, buffer->size);

	usbMutexTake();
	if (!gVendorTxBufferList)
		gVendorTxBufferList = buffer;
	else {
		txBuffer walker;
		for (walker = gVendorTxBufferList; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	usbMutexGive();

	sendNextVendorBuffer();
}

#define PROGRAMMING_TRIGGER_DELAY 500
static uint8_t reboot_style[2] = {0, 0};
static uint32_t reboot_changeTime[2] = {0, 0};
static void checkLineState(uint16_t line_state, uint8_t which) {
	uint32_t now = modMilliseconds();
	uint8_t DTR = (line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR);
	uint8_t RTS = (line_state & APP_USBD_CDC_ACM_LINE_STATE_RTS);
	uint8_t reboot_seq = (DTR ? 1 : 0) + (RTS ? 2 : 0);
//ftdiTraceAndInt2("checkLineState for intf: ", which, line_state);
//ftdiTraceAndInt("   previously: ", reboot_style[which]);


	switch (reboot_seq) {
		case 3:								 // normal run mode
//			ftdiTrace("[3] normal run mode");
			reboot_style[which] = 3;
			break;
		case 2:								 // DTR dropped
			if (which == 1)
				ftdiTrace("[2] ACM - dtr dropped");
			else
				ftdiTrace("[2] VENDOR - dtr dropped");
			reboot_style[which] = 2;
			break;
		case 1:						 // DTR Raised, RTS off - programming mode
			if (which == 1)
				ftdiTrace("[1] ACM - dtr raised, rts dropped - programming mode");
			else
				ftdiTrace("[1] VENDOR - dtr raised, rts dropped - programming mode");
			reboot_style[which] = 1;
			break;
		case 0:
			if ((reboot_style[which] == 1) && ((now - reboot_changeTime[which]) < PROGRAMMING_TRIGGER_DELAY)) {
				if (which == 1) {
					ftdiTrace("[0] ACM - dtr and rts dropped - REBOOT TO PROGRAMMING");
					nrf52_rebootToDFU();
				}
				else {
					ftdiTrace("[0] VENDOR - dtr and rts dropped - REBOOT TO PROGRAMMING - VENDOR ONLY");
					nrf52_rebootToVendor();
				}
			}
			else {
				if (which == 1)
					ftdiTrace("[0] ACM - dtr dropped, rts dropped - RESET");
				else
					ftdiTrace("[0] VENDOR - dtr dropped, rts dropped - RESET");
				nrf52_reset();
			}
			break;
	}

	reboot_changeTime[which] = now;
}

static void vendor_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_vendor_user_event_t event)
{
	ret_code_t ret;
    app_usbd_vendor_t const * p_vendor = app_usbd_vendor_class_get(p_inst);
	uint32_t ulPreviousValue;
	app_usbd_vendor_ctx_t * p_vendor_ctx = vendor_ctx_get(p_vendor);

	checkLineState(p_vendor_ctx->line_state, 0);

    switch(event) {
        case APP_USBD_VENDOR_USER_EVT_PORT_OPEN: {
            // Setup the first transfer.
            // This case is triggered when a remote device connects to the port.
//			ftdiTrace("APP_USBD_VENDOR_USER_EVT_PORT_OPEN");
// queue up a read
            app_usbd_vendor_read(&m_app_vendor, m_vendor_rx_buffer, 1);
//			m_vendor_rx_buffer_pos = 1;	// gets inc'd in RX_DONE
            m_vendor_usb_connected = true;
#if NRF_USBD_REQUIRE_CLOSED_ON_PORT_OPEN
            if (!m_vendor_usb_closed)
                break;
#endif
            m_vendor_usb_closed = false;
            m_vendor_usb_reopened = true;
            break;
        }
		case APP_USBD_VENDOR_USER_EVT_PORT_CLOSE:
            ftdiTrace("APP_USBD_VENDOR_USER_EVT_PORT_CLOSE");
    		m_vendor_usb_connected = false;
    		m_vendor_usb_closed = true;
			break;

        case APP_USBD_VENDOR_USER_EVT_TX_DONE:
            ftdiTraceAndInt("APP_USBD_VENDOR_USER_EVT_TX_DONE - mutex take (cur owned:", uxSemaphoreGetCount(gUSBMutex));
			usbMutexTake();
			if (NULL != gVendorTxBufferList) {
				txBuffer buffer = gVendorTxBufferList;
				gVendorTxBufferList = gVendorTxBufferList->next;
				c_free(buffer);
			}
			
    		if (!m_vendor_usb_connected) {
				ftdiTrace("VENDOR not connected - ignore tx done");
				usbMutexGive();
    			break;
			}
    		
			if (NULL == gVendorTxBufferList && m_vendor_tx_buffer_index > 0) {
				// no TxBufferList, but data in queue - send
				txBuffer buffer = c_calloc(sizeof(txBufferRecord) + m_vendor_tx_buffer_index, 1);
				if (NULL == buffer) {
					usbMutexGive();
					return;	
				}
				buffer->size = m_vendor_tx_buffer_index;
				c_memmove(&buffer->buffer[0], m_vendor_tx_buffer, buffer->size);
//ftdiTraceAndInt("adding incomplete buffer to vendorTxBufferList sized:", buffer->size);
//ftdiTraceHex(buffer->buffer, buffer->size);
				m_vendor_tx_buffer_index = 0;
				gVendorTxBufferList = buffer;
			}
			usbMutexGive();

			ret = sendNextVendorBuffer();
			if (0 != ret)
    			ftdiTrace("sendNextVendorBuffer failed");

			break;

        case APP_USBD_VENDOR_USER_EVT_RX_DONE: {
    		// The first byte is already in the buffer due to
			// app_usbd_vendor_read() in the USER_EVT_PORT_OPEN event.
			// Subsequent iopending reads leave a buffer to be filled.
			uint16_t count = 0;
			if (!nrf_queue_is_full(&m_vendor_rx_queue)) {
				nrf_queue_in(&m_vendor_rx_queue, &m_vendor_rx_buffer[0], 1);
				++count;
			}
			while (!nrf_queue_is_full(&m_vendor_rx_queue)) {
				ret = app_usbd_vendor_read(&m_app_vendor, &m_vendor_rx_buffer[0], 1);
				if (NRF_SUCCESS != ret)
					break;
				nrf_queue_in(&m_vendor_rx_queue, &m_vendor_rx_buffer[0], 1);
				++count;
			}
			
			// here, we inform the xsMain task that there is data available.
			// If ulPreviousValue isn't 0, then data hasn't yet been serviced.
			xTaskNotifyAndQuery(gMainTask, 1, eSetBits, &ulPreviousValue);

			// Drain and toss remaining bytes if queue is full
			if (nrf_queue_is_full(&m_rx_queue)) {
				ftdiTrace("vendor rx queue is full!");
				do {
					ret = app_usbd_vendor_read(&m_app_vendor, &m_vendor_rx_buffer, 1);
				} while (NRF_SUCCESS == ret);
			}
            break;
		}

		case VENDOR_REQUEST_LINE_STATE:
			ftdiTraceAndInt(">> VENDOR - SET CONTROL_LINE_STATE:", p_vendor_ctx->line_state);
			break;
		default:
			break;
	}
}

void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
	ret_code_t ret;
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
	uint32_t ulPreviousValue;
	app_usbd_cdc_acm_ctx_t * p_cdc_acm_ctx = cdc_acm_ctx_get(p_cdc_acm);

	checkLineState(p_cdc_acm_ctx->line_state, 1);

    switch(event) {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
			// Setup the first transfer.
			// This case is triggered when a remote device connects to the port.
//    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN");
			app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, 1);
			m_usb_connected = true;
			break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
//    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_RX_DONE");

    		// The first byte is already in the buffer due to app_usbd_cdc_acm_read()
    		// in the APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN event.
    		uint16_t count = 0;
    		if (!nrf_queue_is_full(&m_rx_queue)) {
//				ftdiTraceChar(m_rx_buffer[0]);
				nrf_queue_in(&m_rx_queue, &m_rx_buffer[0], 1);
				++count;
    		}
			while (!nrf_queue_is_full(&m_rx_queue)) {
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, 1);
                if (NRF_SUCCESS != ret)
                	break;
//				ftdiTraceChar(m_rx_buffer[0]);
				nrf_queue_in(&m_rx_queue, &m_rx_buffer[0], 1);
				++count;
			}
//    		ftdiTraceAndInt("Read bytes =", count);
//			ftdiTraceHex(m_rx_buffer, count);

			// let the usb_debug_task know that there is data to process
			uint32_t val = 1;
			if (pdPASS != xQueueSend(uartQueue, (void*)&val, (TickType_t)10) ) {
				ftdiTrace("Send to uartQueue failed\n");
			}
    		
    		// Drain and toss remaining bytes if queue is full
    		if (nrf_queue_is_full(&m_rx_queue)) {
    			ftdiTrace("Queue is full!");
    			do {
                	ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, 1);
    			} while (NRF_SUCCESS == ret);
    		}
            break;
        }
        
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
//    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE");
    		m_usb_connected = false;
        	break;
        	
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE: {
//    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_TX_DONE");
			usbMutexTake();
			if (NULL != gTxBufferList) {
				txBuffer buffer = gTxBufferList;
				gTxBufferList = gTxBufferList->next;
				c_free(buffer);
			}
			usbMutexGive();
			
    		if (!m_usb_connected || m_vendor_usb_connected)
    			break;
    		
			if (NULL == gTxBufferList && m_tx_buffer_index > 0) {
				// data remaining
				usbMutexTake();
				txBuffer buffer = c_calloc(sizeof(txBufferRecord) + m_tx_buffer_index, 1);
				if (NULL == buffer) {
					usbMutexGive();
					return;
				}
				buffer->size = m_tx_buffer_index;
				c_memmove(&buffer->buffer[0], m_tx_buffer, buffer->size);
				m_tx_buffer_index = 0;
//ftdiTraceAndInt("sending remaining buffer sized:", buffer->size);
//ftdiTraceHex(buffer->buffer, buffer->size);
				gTxBufferList = buffer;
				usbMutexGive();
			}

			ret = sendNextBuffer();
			if (0 != ret)
    			ftdiTrace("FAIL-sendNextBuffer");
        	break;
        }

        default:
            break;
    }
}

#ifdef mxDebug		// moved from way-up. We want usb for the programmer

void ESP_putc(int c)
{
	uint8_t ch = c;

	if (m_vendor_usb_connected) {
		usbMutexTake();
		m_vendor_tx_buffer[m_vendor_tx_buffer_index++] = ch;
	
		// Flush buffer at end of message or when we've reached the buffer size limit
		if ('\n' == ch || kTXBufferSize == m_vendor_tx_buffer_index) {
			uint8_t doSend = (NULL == gVendorTxBufferList);

			txBuffer buffer = c_calloc(sizeof(txBufferRecord) + m_vendor_tx_buffer_index, 1);
			if (NULL == buffer) {
				usbMutexGive();
				return;
			}
		
			buffer->size = m_vendor_tx_buffer_index;
			c_memmove(&buffer->buffer[0], m_vendor_tx_buffer, buffer->size);
			m_vendor_tx_buffer_index = 0;

			if (!gVendorTxBufferList)
				gVendorTxBufferList = buffer;
			else {
				txBuffer walker;
				for (walker = gVendorTxBufferList; walker->next; walker = walker->next)
					;
				walker->next = buffer;
			}

			if (doSend)
				sendNextVendorBuffer();
		}
		usbMutexGive();
	}
	else if (m_usb_connected) {
		usbMutexTake();
		m_tx_buffer[m_tx_buffer_index++] = ch;
	
		// Flush buffer at end of message or when we've reached the buffer size limit
		if ('\n' == ch || kTXBufferSize == m_tx_buffer_index) {
			uint8_t doSend = (NULL == gTxBufferList);
		
			txBuffer buffer = c_calloc(sizeof(txBufferRecord) + m_tx_buffer_index, 1);
			if (NULL == buffer) {
				usbMutexGive();
				return;
			}
		
			buffer->size = m_tx_buffer_index;
			c_memmove(&buffer->buffer[0], m_tx_buffer, buffer->size);
			m_tx_buffer_index = 0;

			if (!gTxBufferList)
				gTxBufferList = buffer;
			else {
				txBuffer walker;
				for (walker = gTxBufferList; walker->next; walker = walker->next)
					;
				walker->next = buffer;
			}

			if (doSend)
				sendNextBuffer();
		}
		usbMutexGive();
	}
}

int ESP_getc(void)
{
	uint8_t ch;
	ret_code_t ret;
	size_t size;

	if (m_vendor_usb_connected) {
		if (/*gTxBufferList &&*/ nrf_queue_is_empty(&m_vendor_rx_queue)) {
			taskYIELD();
			goto bail;
		}

		size = nrf_queue_out(&m_vendor_rx_queue, &ch, 1);
	}
	else if (m_usb_connected) {
		if (/*gTxBufferList &&*/ nrf_queue_is_empty(&m_rx_queue)) {
			taskYIELD();
			goto bail;
		}

		size = nrf_queue_out(&m_rx_queue, &ch, 1);
	}
	else {
		if (nrf_queue_is_empty(&m_rx_queue))
			goto bail;

		ftdiTrace(" - getc (not connected) - data remaining");
		size = nrf_queue_out(&m_rx_queue, &ch, 1);
	}

    if (1 == size) {
		return (int)ch;
    }
	
bail:
	return -1;
}

#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }

#endif

#endif	// USE_DEBUGGER_USBD
