/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#if USE_DEBUGGER_USBD

#ifdef mxDebug

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

#include "task.h"
#include "queue.h"
#include "ftdi_trace.h"

// LED to blink when device is ready for host serial connection
#define LED 7

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);
static void usbd_user_ev_handler(app_usbd_event_type_t event);
static void usbd_task(void *pvParameter);
static void usb_new_event_isr_handler(app_usbd_internal_evt_t const * const p_event, bool queued);

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

#define USBD_STACK_SIZE			256
#define USBD_PRIORITY			2
#define USB_THREAD_MAX_BLOCK_TIME portMAX_DELAY

#define kTXBufferSize 2048

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
NRF_QUEUE_DEF(uint8_t, m_rx_queue, 2048, NRF_QUEUE_MODE_OVERFLOW);

static uint8_t m_usb_connected = false;
static uint8_t m_usb_restarted = false;
static uint8_t m_usb_reopened = false;
static uint8_t m_usb_closed = false;

static TaskHandle_t m_usbd_thread;

static void blink(uint16_t times)
{
	nrf_gpio_cfg_output(LED);
	for (uint16_t i = 0; i < times; ++i) {
		nrf_gpio_pin_write(LED, 0);
		modDelayMilliseconds(100);
		nrf_gpio_pin_write(LED, 1);
		modDelayMilliseconds(100);
	}
	nrf_gpio_pin_write(LED, 0);
}

void setupDebugger()
{
	uint32_t count;

	m_usb_connected = false;
	m_usb_restarted = false;
	m_usb_reopened = false;
	m_usb_closed = false;

	app_usbd_serial_num_generate();

	if (pdPASS != xTaskCreate(usbd_task, "USBD", USBD_STACK_SIZE, NULL, USBD_PRIORITY, &m_usbd_thread))
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
        
    ftdiTrace("Waiting for USBD port connection");
    
    // Wait two seconds to let USBD port complete the suspend/resume dance
	count = 0;
	while (count++ < 20) {
		taskYIELD();
		modDelayMilliseconds(100);
	}
	//blink(5);

    // Wait up to approximately ten seconds for USBD port to be available and connected to host
    count = 0;
	while (!m_usb_connected && (count++ < 100)) {
		taskYIELD();
		modDelayMilliseconds(100);
	}
	
	// Wait for serial2xsbug to complete host serial port initialization
	if (m_usb_connected) {
		count = 0;
		while (!m_usb_reopened && (count++ < 30)) {
			taskYIELD();
			modDelayMilliseconds(100);
		}
	}
	
	// Finally, let the host connection settle
	modDelayMilliseconds(1000);
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

void usbd_user_ev_handler(app_usbd_event_type_t event)
{
	switch(event) {
		case APP_USBD_EVT_STOPPED:
    		ftdiTrace("APP_USBD_EVT_STOPPED");
			app_usbd_disable();
			break;

		case APP_USBD_EVT_STARTED:
    		ftdiTrace("APP_USBD_EVT_STARTED");
			break;

		case APP_USBD_EVT_POWER_DETECTED:
    		ftdiTrace("APP_USBD_EVT_POWER_DETECTED");
			if (!nrf_drv_usbd_is_enabled()) {
    			ftdiTrace("calling app_usbd_enable");
				app_usbd_enable();
			}
			else
    			ftdiTrace("usbd already enabled");
			break;

		case APP_USBD_EVT_POWER_REMOVED:
    		ftdiTrace("APP_USBD_EVT_POWER_REMOVED");
			app_usbd_stop();
			break;

		case APP_USBD_EVT_POWER_READY:
    		ftdiTrace("APP_USBD_EVT_POWER_READY");
			if (!nrf_drv_usbd_is_enabled()) {
    			ftdiTrace("calling app_usbd_enable");
				app_usbd_enable();
			}
			else
    			ftdiTrace("usbd already enabled");
			app_usbd_start();
			break;

		case APP_USBD_EVT_DRV_SUSPEND:
    		ftdiTrace("APP_USBD_EVT_DRV_SUSPEND");
			break;

		case APP_USBD_EVT_DRV_RESUME:
    		ftdiTrace("APP_USBD_EVT_DRV_RESUME");
			break;

		default:
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

static app_usbd_cdc_acm_ctx_t * cdc_acm_ctx_get(app_usbd_cdc_acm_t const * p_cdc_acm)
{
    ASSERT(p_cdc_acm != NULL);
    ASSERT(p_cdc_acm->specific.p_data != NULL);
    return &p_cdc_acm->specific.p_data->ctx;
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
		ftdiTrace("DTR line not set!");
        return NRF_ERROR_INVALID_STATE;	// port not opened
    }
    
    nrf_drv_usbd_ep_t ep = data_ep_in_addr_get(p_inst);
    
	NRF_DRV_USBD_TRANSFER_IN_ZLP(transfer, &buffer->buffer[0], buffer->size);
	ftdiTrace("sending buffer, size=");
	ftdiTraceInt(buffer->size);
	return app_usbd_ep_transfer(ep, &transfer);
}

void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
	ret_code_t ret;
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
	uint32_t ulPreviousValue;

    switch(event) {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
			// Setup the first transfer.
			// This case is triggered when a remote device connects to the port.
    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN");
			app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, 1);
			m_usb_connected = true;
			if (m_usb_closed) {
				m_usb_closed = false;
				m_usb_reopened = true;
			}
            break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_RX_DONE");
    		
    		// The first byte is already in the buffer due to app_usbd_cdc_acm_read()
    		// in the APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN event.
    		uint16_t count = 0;
    		if (!nrf_queue_is_full(&m_rx_queue)) {
				ftdiTraceChar(m_rx_buffer[0]);
				nrf_queue_in(&m_rx_queue, &m_rx_buffer[0], 1);
				++count;
    		}
			while (!nrf_queue_is_full(&m_rx_queue)) {
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer, 1);
                if (NRF_SUCCESS != ret)
                	break;
				ftdiTraceChar(m_rx_buffer[0]);
				nrf_queue_in(&m_rx_queue, &m_rx_buffer[0], 1);
				++count;
			}
    		ftdiTrace("Read bytes =");
    		ftdiTraceInt(count);

			// Here, we inform the xsMain task that there is data available.
			// If ulPreviousValue isn't 0, then data hasn't yet been serviced.
			xTaskNotifyAndQuery(gMainTask, 1, eSetBits, &ulPreviousValue);
    		
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
    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE");
    		m_usb_connected = false;
    		m_usb_closed = true;
        	break;
        	
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE: {
    		ftdiTrace("APP_USBD_CDC_ACM_USER_EVT_TX_DONE");
			if (NULL != gTxBufferList) {
				txBuffer buffer = gTxBufferList;
				gTxBufferList = gTxBufferList->next;
				c_free(buffer);
			}
			
    		if (!m_usb_connected)
    			break;
    		
			ret = sendNextBuffer();
			if (0 != ret)
    			ftdiTrace("sendNextBuffer failed");
        	break;
        }
        	
        default:
            break;
    }
}

void modLog_transmit(const char *msg)
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
		while (0 != (c = c_read8(msg++)))
			ESP_putc(c);
	ESP_putc('\r');
	ESP_putc('\n');
}

void ESP_putc(int c)
{
	uint8_t ch = c;

	if (!m_usb_connected)
		return;

	m_tx_buffer[m_tx_buffer_index++] = ch;
	
	// Flush buffer at end of message or when we've reached the buffer size limit
	if ('\n' == ch || kTXBufferSize == m_tx_buffer_index) {
		uint8_t doSend = (NULL == gTxBufferList);
		
		txBuffer buffer = c_calloc(sizeof(txBufferRecord) + m_tx_buffer_index, 1);
		if (NULL == buffer) return;
		
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

		if (doSend) {
			sendNextBuffer();
		}
		else {
			ftdiTrace("queuing buffer");
		}
	}
}

int ESP_getc(void)
{
	uint8_t ch;
	ret_code_t ret;
	size_t size;

	if (!m_usb_connected)
		return -1;

	if (/*gTxBufferList &&*/ nrf_queue_is_empty(&m_rx_queue)) {
		taskYIELD();
		goto bail;
	}

	size = nrf_queue_out(&m_rx_queue, &ch, 1);
    if (1 == size) {
		return (int)ch;
    }
	
bail:
	return -1;
}

#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
void modLog_transmit(const char *msg) { }

#endif

#endif	// USE_DEBUGGER_USBD
