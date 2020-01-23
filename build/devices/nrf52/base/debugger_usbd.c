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
#include "task.h"
#include "queue.h"
#include "sdk_config.h"

#if USE_DEBUGGER_USBD

#if mxDebug

#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_ringbuf.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

typedef struct {
	uint16_t id;
} modDebugTaskQueueRecord;

// Set USE_RX_RING_BUFFER to 1 to read all available data in the USB user event callback into a ring buffer
#define USE_RX_RING_BUFFER 1

// Set USE_TX_RING_BUFFER to 1 to buffer unsent data into a ring buffer to send when TX becomes available
#define USE_TX_RING_BUFFER 1

// Set USE_TX_BUFFERS to 1 to buffer data until the buffer fills or a `\n` is written.
// Note this technique isn't reliable because sometimes there is no trailing `\n` character.
#define USE_TX_BUFFERS 0

// Set USE_DEBUG_TASK to 1 to process USB RX/TX notifications in a separate debug task
#define USE_DEBUG_TASK 1

#if (USE_TX_RING_BUFFER && USE_TX_BUFFERS)
	#error - only one of USE_TX_RING_BUFFER or USE_TX_BUFFERS can be set to 1
#endif

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);
static void usbd_user_ev_handler(app_usbd_event_type_t event);
static void debug_task(void *pvParameter);
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

#define DEBUGGER_STACK			768
#define DEBUG_QUEUE_LEN			8
#define DEBUG_QUEUE_ITEM_SIZE	(sizeof(modDebugTaskQueueRecord))

enum {
	DEBUG_TASK_CREATED = 1,		// task running
	DEBUG_USBD_AVAILABLE,		// usbd port available for host
	DEBUG_USBD_CONNECTED,		// host connected to usbd port
	DEBUG_READABLE,				// data available to read
	DEBUG_WRITABLE,				// data can be written to port
	DEBUG_DRV_ERR
};

#if USE_DEBUG_TASK
	static QueueHandle_t gDebugTaskQueue;
#endif

static TaskHandle_t m_usbd_thread;
static bool m_usb_connected = false;
static bool m_tx_pending = false;

static uint16_t m_tx_written = 0;
static uint16_t m_tx_acked = 0;
static uint16_t m_rx_received = 0;
static uint16_t m_rx_read = 0;

#if USE_TX_BUFFERS
#define kTXBufferCount 1024
#define kTXBufferSize 1
static uint8_t m_tx_buffers[kTXBufferCount][kTXBufferSize];
static uint16_t m_tx_buffer = 0;
static uint16_t m_tx_buffer_index = 0;
#endif

#if USE_TX_RING_BUFFER
	static uint8_t m_tx_buffer;	// buffer content cannot change until APP_USBD_CDC_ACM_USER_EVT_TX_DONE confirmation
	NRF_RINGBUF_DEF(m_tx_ringbuf, 1024);
#endif

#if USE_RX_RING_BUFFER
	NRF_RINGBUF_DEF(m_rx_ringbuf, 1024);
#endif

int debuggerStats()
{
	uint16_t tx_written = m_tx_written;
	uint16_t tx_acked = m_tx_acked;
	uint16_t rx_received = m_rx_received;
	uint16_t rx_read = m_rx_read;
	if (tx_acked != tx_written)
		return -1;
}

void setupDebugger()
{
	m_tx_written = 0;
	m_tx_acked = 0;
	m_rx_received = 0;
	m_rx_read = 0;
	
#if USE_TX_RING_BUFFER
	nrf_ringbuf_init(&m_tx_ringbuf);
#endif
#if USE_RX_RING_BUFFER
	nrf_ringbuf_init(&m_rx_ringbuf);
#endif
	
#if USE_DEBUG_TASK
	gDebugTaskQueue = xQueueCreate(DEBUG_QUEUE_LEN, DEBUG_QUEUE_ITEM_SIZE);
	if (pdPASS != xTaskCreate(debug_task, "debug", DEBUGGER_STACK/sizeof(StackType_t), NULL, 8, NULL))
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
#endif

	app_usbd_serial_num_generate();

    if (pdPASS != xTaskCreate(usbd_task, "USBD", USBD_STACK_SIZE, NULL, USBD_PRIORITY, &m_usbd_thread))
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
        
    // Wait up to approximately ten seconds for USBD port to be available and connected to host
	uint32_t count = 0;
	while (!m_usb_connected && (count++ < 1000)) {
		taskYIELD();
		modDelayMilliseconds(10);
	}
	if (m_usb_connected)
		modDelayMilliseconds(100);
}

void usb_new_event_isr_handler(app_usbd_internal_evt_t const * const p_event, bool queued)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UNUSED_PARAMETER(p_event);
    UNUSED_PARAMETER(queued);
    ASSERT(m_usbd_thread != NULL);
    /* Release the semaphore */
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
    APP_ERROR_CHECK(ret);
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);

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
			app_usbd_disable();
			break;

		case APP_USBD_EVT_STARTED:
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

		case APP_USBD_EVT_DRV_SUSPEND:
			break;

		case APP_USBD_EVT_DRV_RESUME:
			break;

		default:
			break;
	}
}

void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
	ret_code_t ret;
	uint8_t c;
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
#if USE_DEBUG_TASK
	modDebugTaskQueueRecord msg = {0};
#endif

    switch(event) {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
			// Setup the first transfer - this is called once when a remote device connects to the port
            ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, &c, 1);
#if USE_DEBUG_TASK
			msg.id = DEBUG_USBD_CONNECTED;
#else
			m_usb_connected = true;
#endif
            break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
        	size_t index = 0;
#if USE_RX_RING_BUFFER
			size_t len;
            do {
            	len = 1;
				ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, &c, len);
                if (ret == NRF_SUCCESS) {
					nrf_ringbuf_cpy_put(&m_rx_ringbuf, &c, &len);
					++index;
					++m_rx_received;
                }
            }
            while (ret == NRF_SUCCESS);
#endif
#if USE_DEBUG_TASK
			if (0 != index)
				msg.id = DEBUG_READABLE;
#endif
            break;
        }
        
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        	break;
        	
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE: {
#if USE_TX_RING_BUFFER
			size_t len = 1;
			uint8_t ch;
			++m_tx_acked;
			ret = nrf_ringbuf_cpy_get(&m_tx_ringbuf, &ch, &len);
			if (1 == len) {
				m_tx_buffer = ch;
				ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, &m_tx_buffer, 1);
				++m_tx_written;
			}
			else
				m_tx_pending = false;		
#elif USE_TX_BUFFERS
			msg.id = DEBUG_WRITABLE;
#endif
        	break;
        }
        	
        default:
            break;
    }
    
#if USE_DEBUG_TASK
	if (0 != msg.id)
		xQueueSend(gDebugTaskQueue, &msg, NULL);
#endif
}

#if USE_DEBUG_TASK
void debug_task(void *pvParameter)
{
	modDebugTaskQueueRecord msg;

	msg.id = DEBUG_TASK_CREATED;
	xQueueSend(gDebugTaskQueue, &msg, 0);

	while (true) {
		if (!xQueueReceive(gDebugTaskQueue, (void*)&msg, portMAX_DELAY))
			continue;

		switch(msg.id) {
			case DEBUG_USBD_AVAILABLE:
				break;
			case DEBUG_USBD_CONNECTED:
				m_usb_connected = true;
				break;
			case DEBUG_READABLE:
				fxReceiveLoop();
				break;
			case DEBUG_WRITABLE:
				break;
			case DEBUG_DRV_ERR:
				break;
			default:
				break;
		}
	}
}
#endif

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
	uint8_t ch = c;
	
#if USE_TX_BUFFERS
	m_tx_buffers[m_tx_buffer][m_tx_buffer_index++] = ch;
	if ('\n' == ch || kTXBufferSize == m_tx_buffer_index) {
		app_usbd_cdc_acm_write(&m_app_cdc_acm, &(m_tx_buffers[m_tx_buffer][0]), m_tx_buffer_index);
		m_tx_buffer = (m_tx_buffer + 1) % kTXBufferCount;
		m_tx_buffer_index = 0;
	}
#elif USE_TX_RING_BUFFER
	size_t len = 1;
    if (m_tx_pending) {
		nrf_ringbuf_cpy_put(&m_tx_ringbuf, &ch, &len);
    }
    else {
    	m_tx_buffer = ch;
		ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, &m_tx_buffer, 1);
		m_tx_pending = true;
		++m_tx_written;
    }
#else
	ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, &ch, 1);
	while (NRF_ERROR_BUSY == ret) {
		taskYIELD();	// @@
		ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, &ch, 1);
	}
	if (NRF_SUCCESS == ret) {
		++m_tx_written;
	}
	else {
		int foo = 1;
	}
#endif
}

int ESP_getc(void) {
	uint8_t ch;
	ret_code_t ret;

#if USE_TX_RING_BUFFER
	if (m_tx_pending) {
		vTaskDelay(1);
		return -1;
	}
#endif

#if USE_RX_RING_BUFFER
	size_t len = 1;
	ret = nrf_ringbuf_cpy_get(&m_rx_ringbuf, &ch, &len);
	if (1 == len) {
		++m_rx_read;
		return ch;
	}
#else
	uint16_t i;
	#define kTries 5
	for (i = 0; i < kTries; ++i) {
		size_t len = 1;
		ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, &ch, 1);
		if (NRF_SUCCESS == ret) {
			return ch;
		}
		else if (NRF_ERROR_IO_PENDING == ret) {
			taskYIELD();	// @@
		}
		else
			break;
    }
#endif

	return -1;
}



#else

void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }
void modLog_transmit(const char *msg) { }

#endif

#endif	// USE_DEBUGGER_USBD