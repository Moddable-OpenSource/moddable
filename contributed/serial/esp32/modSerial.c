/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include "mc.defines.h"
#include "modSerial.h"
#include "xsesp.h"

#include "driver/uart.h"

static uint8_t modSerialActivate(modSerialConfiguration config);

void modSerialInit(modSerialConfiguration config)
{
	modSerialActivate(config);
}

void modSerialUninit(modSerialConfiguration config)
{
	uart_driver_delete(UART_NUM_1);
}

uint8_t modSerialRead(modSerialConfiguration config, uint8_t *buffer, uint16_t length)
{
	int ret;
	
	ret = uart_read_bytes(UART_NUM_1, buffer, length, 100);

	return (ret > 0) ? 0 : 1;
}

uint8_t modSerialWrite(modSerialConfiguration config, const uint8_t *buffer)
{
	int ret;

	ret = uart_write_bytes(UART_NUM_1, buffer, strlen(buffer));

	return (ret > 0) ? 0 : 1;
}

uint8_t modSerialActivate(modSerialConfiguration config)
{
	modLog("Activating uart");
	uart_config_t conf;

	conf.baud_rate = config->baudrate;
	conf.data_bits = UART_DATA_8_BITS;
	conf.parity    = UART_PARITY_DISABLE;
	conf.stop_bits = UART_STOP_BITS_1;
	conf.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	conf.rx_flow_ctrl_thresh = 122;

	if (ESP_OK != uart_param_config(UART_NUM_1, &conf)) {
		modLog("uart_param_config fail");
		return 1;
	}

	if (ESP_OK != uart_set_pin(UART_NUM_1, config->tx, config->rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) {
		modLog("uart_set_pin fail");
		return 1;
	}

	if (ESP_OK != uart_driver_install(UART_NUM_1, 1024, 1024, 10, NULL, 0)) {
		modLog("uart_driver_install fail");
		return 1;
	}

	return 0;
}

