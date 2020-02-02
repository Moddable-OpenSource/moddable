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

#include "ftdi_trace.h"
#include "nrfx_uart.h"

#if USE_FTDI_TRACE 

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

void ftdiTraceInit() {
	ret_code_t ret;

	ret = nrfx_uart_init(&gDebuggerUart, &gDebuggerUartConfig, NULL);
}

static void ftdiTx(uint8_t *buffer)
{
	while (nrfx_uart_tx_in_progress(&gDebuggerUart))
		taskYIELD();

	nrfx_uart_tx(&gDebuggerUart, buffer, c_strlen(buffer));
}

void ftdiTrace(const char *msg)
{
	uint8_t buffer[128];

	snprintf(buffer, sizeof(buffer), "%s\n", msg);
	ftdiTx(buffer);
}

void ftdiTraceInt(int i)
{
	uint8_t buffer[128];

	snprintf(buffer, sizeof(buffer), "%d\n", i);
	ftdiTx(buffer);
}

void ftdiTraceChar(int c)
{
	uint8_t buffer[128];

	if (isprint(c))
		snprintf(buffer, sizeof(buffer), "%c\n", c);
	else
		snprintf(buffer, sizeof(buffer), "0x%02X\n", (uint16_t)c);
	ftdiTx(buffer);
}

#else
	void ftdiTraceInit() {}
	void ftdiTrace(const char *msg) {}
	void ftdiTraceInt(int i) {}
	void ftdiTraceChar(int c) {}
#endif
