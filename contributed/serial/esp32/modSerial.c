/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "xsHost.h"

#include "mc.defines.h"

#include "modGPIO.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_attr.h"
#include "driver/uart.h"
#include "sdkconfig.h"

#include "../modSerial.h"

typedef struct modSerialDeviceRecord {
	uart_port_t			uart;
	uart_config_t		config;
	uart_isr_handle_t	handler;
	uint8_t				is_setup;
	intr_handle_t		intr_handle;
	int					timeoutMS;
	int					timeoutEndMS;
	QueueHandle_t		queue;

	// for poll (or interrupt in this case)
	TaskHandle_t		task;
	char				terminators[MAX_TERMINATORS];
	int					numTerminators;
	uint8_t				trim;
	modSerialPollCallbackFn	poll_callback;
	uint8_t				*data;
	int					dataLen;
	int					dataPos;
	void				*refcon;
} modSerialDeviceRecord, *modSerialDevice;


static void modSerialEventTask(void *pvParameters) {
	modSerialDevice serial = (modSerialDevice)pvParameters;
	uart_event_t event;
	size_t buffered_size;
	int i, j;
	uint8_t c;

	for (;;) {
		if (xQueueReceive(serial->queue, (void*)&event, (portTickType)portMAX_DELAY)) {
			switch(event.type) {
				case UART_DATA:
					while (uart_read_bytes(serial->uart, serial->data + serial->dataPos, 1, portMAX_DELAY)) {
						c = *(serial->data + serial->dataPos);
						serial->dataPos++;
						for (j=0; j<serial->numTerminators; j++) {
							if (c == serial->terminators[j]) {
								// got a terminator
								if (serial->trim) {
									*(serial->data + serial->dataPos) = '\0';
									serial->dataPos--;
								}

								// make the callback
								if (serial->dataPos > 0)
									(serial->poll_callback)(serial->data, serial->dataPos, serial->refcon);
								serial->dataPos = 0;
							}
						}
						if (serial->dataPos == serial->dataLen) {
							// hit the end.
							// make the callback
							(serial->poll_callback)(serial->data, serial->dataPos, serial->refcon);
							serial->dataPos = 0;
						}
						
					}
					break;
/*
				case UART_BREAK:
					debugout("uart event UART_BREAK (%d)", event.type);
					break;
				case UART_BUFFER_FULL:
					debugout("uart event UART_BUFFER_FULL (%d)", event.type);
					break;
				case UART_FIFO_OVF:
					debugout("uart event UART_FIFO_OVF (%d)", event.type);
					break;
				case UART_FRAME_ERR:
					debugout("uart event UART_FRAME_ERR (%d)", event.type);
					break;
				case UART_PARITY_ERR:
					debugout("uart event UART_PARITY_ERR (%d)", event.type);
					break;
				case UART_DATA_BREAK:
					debugout("uart event UART_DATA_BREAK (%d)", event.type);
					break;
				case UART_PATTERN_DET:
					debugout("uart event UART_PATTERN_DET (%d)", event.type);
					break;
*/
				default:
					debugout("unhandled uart event (%d)", event.type);
					break;
			}
		}
	}
}

void modSerial_poll(modSerialDevice serial, int interval, char *terminators, uint8_t trim, uint8_t *data, int dataLen, modSerialPollCallbackFn callback, void *refcon) {
	int err;
	c_memcpy(serial->terminators, terminators, 16);
	serial->numTerminators = c_strlen(terminators);
	serial->trim = trim;
	serial->data = data;
	serial->dataPos = 0;
	serial->dataLen = dataLen;
	serial->poll_callback = callback;
	serial->refcon = refcon;
	err = uart_enable_rx_intr(serial->uart);
	if (err) debugout("uart_enable_rx_intr err %d", err);

	xTaskCreate(modSerialEventTask, "modSerial", 2048/sizeof(StackType_t), serial, 10, &serial->task);
}

void modSerial_endPoll(modSerialDevice serial) {
	int err;
	if (serial->task) {
		vQueueDelete(serial->queue);
		vTaskDelete(serial->task);
	}
	serial->task = NULL;
	err = uart_disable_rx_intr(serial->uart);
}

void modSerial_puts(modSerialDevice serial, uint8_t *buf, int len) {
	esp_err_t ret;

	uart_write_bytes(serial->uart, buf, len);
	uart_wait_tx_done(serial->uart, serial->timeoutMS);
}

void modSerial_putc(modSerialDevice serial, int c) {
	uint8_t data = c;
	modbusSerial_puts(serial, &data, 1);
}

int modSerial_gets(modSerialDevice serial, uint8_t *buf, int len) {
	return uart_read_bytes(serial->uart, buf, len, serial->timeoutMS);
}

int modSerial_getc(modSerialDevice serial) {
	int ret;
	uint8_t data[2];

	ret = uart_read_bytes(serial->uart, data, 1, 0);
	if (1 == ret)
		return data[0];
	return -1;
}


void modSerial_setTimeout(modSerialDevice serial, int timeoutMS) {
	serial->timeoutMS = timeoutMS;
}

int modSerial_read(modSerialDevice serial, uint8_t *buf, int len) {
	int ret, amt = len;
	char *p = buf;
	uint32_t millis = modMilliseconds();
	serial->timeoutEndMS = millis + serial->timeoutMS;

	while (amt) {
		int timeout = serial->timeoutEndMS - millis;
		ret = uart_read_bytes(serial->uart, p, amt, timeout);
		if (ret > 0) {
			p += ret;
			amt -= ret;
		}
		millis = modMilliseconds();
		if (millis > serial->timeoutEndMS)
			break;
	}

	return len - amt;
}

int modSerial_write(modSerialDevice serial, uint8_t *buf, int len) {
	int ret, amt = len;
	uint8_t *p = buf;
	serial->timeoutEndMS = modMilliseconds() + serial->timeoutMS;

	while (amt) {
		ret = uart_write_bytes(serial->uart, p, amt);
		if (ret > 0) {
			p += ret;
			amt -= ret;
		}
		if (modMilliseconds() > serial->timeoutEndMS)
			break;
	}

	return len - amt;
}


void modSerial_flush(modSerialDevice serial) {
	uint8_t data[2];
	int ret;
	do {
		ret = uart_read_bytes(serial->uart, data, 1, 0);
	} while (ret > 0);
}

int modSerial_available(modSerialDevice serial) {
	int ret;
	if (0 == uart_get_buffered_data_len(serial->uart, &ret))
		return ret;
	return 0;
}

void modSerial_setBaudrate(modSerialDevice serial, int speed) {
	uart_set_baudrate(serial->uart, speed);
}

int modSerial_peek(modSerialDevice serial) {
	return 0;
}

int modSerial_txBusy(modSerialDevice serial) {
	// we do blocking write
	return false;
}

void modSerial_teardown(modSerialDevice serial) {
	if (serial->task) {
		vQueueDelete(serial->queue);
		vTaskDelete(serial->task);
	}
	uart_driver_delete(serial->uart);
	c_free(serial);
}

modSerialDevice modSerialDevice_setup(modSerialConfig config) {
	esp_err_t			err;
	modSerialDevice 	serial;

	serial = c_calloc(1, sizeof(modSerialDeviceRecord));

	serial->config.baud_rate = config->baud;

	if (config->interface == 0)
		serial->uart = UART_NUM_0;
	else if (config->interface == 1)
		serial->uart = UART_NUM_1;
#if SOC_UART_NUM > 2
	else if (config->interface == 2)
		serial->uart = UART_NUM_2;
#endif
	else
		serial->uart = UART_NUM_0;

	if (config->databits == 5)
		serial->config.data_bits = UART_DATA_5_BITS;
	else if (config->databits == 6)
		serial->config.data_bits = UART_DATA_6_BITS;
	else if (config->databits == 7)
		serial->config.data_bits = UART_DATA_7_BITS;
	else
		serial->config.data_bits = UART_DATA_8_BITS;

	if (config->parity[0] == 'E' || config->parity[0] == 'e') {
		serial->config.parity = UART_PARITY_EVEN;
	}
	else if (config->parity[0] == 'O' || config->parity[0] == 'o') {
		serial->config.parity = UART_PARITY_ODD;
	}
	else
		serial->config.parity = UART_PARITY_DISABLE;

	if (config->stopbits == 1) {
		serial->config.stop_bits = UART_STOP_BITS_1;
	}
	else if (config->stopbits == 15) {
		serial->config.stop_bits = UART_STOP_BITS_1_5;
	}
	else if (config->stopbits == 2) {
		serial->config.stop_bits = UART_STOP_BITS_2;
	}

	serial->config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
//	serial->config.flow_ctrl = UART_HW_FLOWCTRL_RTS;
	serial->config.rx_flow_ctrl_thresh = 122;

	err = uart_param_config(serial->uart, &(serial->config));
	if (err) debugout("uart_param_config err: %d", err);

	err = uart_set_pin(serial->uart, config->txPin, config->rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err) debugout("uart_set_pin err: %d", err);

	err = uart_driver_install(serial->uart, config->rxBufferSize, config->txBufferSize, 20, &serial->queue, 0);
	if (err) debugout("uart_driver_install err: %d", err);

	return serial;
}

