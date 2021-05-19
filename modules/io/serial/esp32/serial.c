/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

/*
	to do:
		!! format: ascii
		onError - framing errors?
		drain transmit? (onWritable when threshold is 0)
		flush transmit, flush receive?
		other pins, hardware flow control
*/


#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/uart.h"
#include "hal/uart_hal.h"
#include "hal/uart_types.h"
#include "soc/uart_caps.h"
#include "soc/uart_struct.h"

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

struct SerialRecord {
	xsSlot		obj;
	uint8_t		isReadable;
	uint8_t		isWritable;
	uint8_t		format;
	uint8_t		uart;
	uart_dev_t	*uart_reg;
	uint32_t	transmit;
	uint32_t	receive;
	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
};

static void ICACHE_RAM_ATTR serial_isr(void * arg);
static void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t hasReadable, hasWritable, format;
	esp_err_t err;
	uart_config_t uartConfig = {0};
	int uart = 0;
	uint32_t transmit = UART_PIN_NO_CHANGE, receive = UART_PIN_NO_CHANGE;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_transmit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_transmit);
		transmit = xsmcToInteger(xsVar(0));
		if (!builtinIsPinFree(transmit))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_receive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_receive);
		receive = xsmcToInteger(xsVar(0));
		if (!builtinIsPinFree(receive))
			xsUnknownError("in use");
	}
	else if (UART_PIN_NO_CHANGE == transmit)
		xsUnknownError("invalid");

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		uart = xsmcToInteger(xsVar(0));
		if ((uart < 0) || (uart >= UART_NUM_MAX))
			xsRangeError("invalid port");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	if ((baud < 0) || (baud > 20000000))
		xsRangeError("invalid baud");

	hasReadable = (UART_PIN_NO_CHANGE != receive) && builtinHasCallback(the, xsID_onReadable);
	hasWritable = (UART_PIN_NO_CHANGE != transmit) && builtinHasCallback(the, xsID_onWritable);

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	if (uart_is_driver_installed(uart))
		xsRangeError("port in use");

	uartConfig.baud_rate = baud;
	uartConfig.data_bits = UART_DATA_8_BITS;
	uartConfig.parity = UART_PARITY_DISABLE;
	uartConfig.stop_bits = UART_STOP_BITS_1;
	uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uartConfig.rx_flow_ctrl_thresh = 120;		// unused. no hardware flow control.
//	uartConfig.use_ref_tick = 0;	 // deprecated in 4.x

	err = uart_param_config(uart, &uartConfig);
	if (err)
		xsUnknownError("uart failed");

	err = uart_set_pin(uart, transmit, receive, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err)
		xsUnknownError("uart failed");

	serial = c_malloc((hasReadable || hasWritable) ? sizeof(SerialRecord) : offsetof(SerialRecord, the));
	if (!serial)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, serial);

	serial->obj = xsThis;

	serial->transmit = transmit;
	serial->receive = receive;
	serial->format = format;
	serial->uart = (uint8_t)uart;
	serial->isReadable = 0;
	serial->isWritable = 0;
#if UART_NUM_MAX > 2
	if (2 == uart)
		serial->uart_reg = &UART2;
	else
#endif
		serial->uart_reg = uart ? &UART1 : &UART0;

	if (hasReadable || hasWritable) {
		serial->the = the;

		xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

		// store callbacks & configure interrupts
		err = uart_isr_register(uart, serial_isr, serial, 0, NULL);
		if (err)
			xsUnknownError("uart_isr_register failed");

		if (hasReadable) {
			builtinGetCallback(the, xsID_onReadable, &xsVar(0));
			serial->onReadable = xsToReference(xsVar(0));

			uart_enable_rx_intr(uart);
			uart_set_rx_timeout(uart, 4);
		}
		else {
			uart_disable_rx_intr(uart);
			serial->onReadable = NULL;
		}

		if (hasWritable) {
			builtinGetCallback(the, xsID_onWritable, &xsVar(0));
			serial->onWritable = xsToReference(xsVar(0));

			uart_enable_tx_intr(uart, 1, 20);
		}
		else {
			uart_disable_tx_intr(uart);
			serial->onWritable = NULL;
		}
	}

	xsRemember(serial->obj);

	if (UART_PIN_NO_CHANGE != transmit)
		builtinUsePin(transmit);
	if (UART_PIN_NO_CHANGE != receive)
		builtinUsePin(receive);
}

void xs_serial_destructor(void *data)
{
	Serial serial = data;
	if (!serial) return;

	uart_ll_set_txfifo_empty_thr(serial->uart_reg, 0);
	uart_enable_tx_intr(serial->uart, 0, 0);
	uart_set_rx_timeout(serial->uart, 0);
	uart_disable_rx_intr(serial->uart);

	uart_isr_free(serial->uart);

	if (UART_PIN_NO_CHANGE != serial->transmit)
		builtinFreePin(serial->transmit);
	if (UART_PIN_NO_CHANGE != serial->receive)
		builtinFreePin(serial->receive);

	c_free(serial);
}

void xs_serial_close(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	if (!serial) return;

	xsmcSetHostData(xsThis, NULL);
	xsForget(serial->obj);
	xs_serial_destructor(serial);
}

void xs_serial_get_format(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	if (!serial) return;
	builtinGetFormat(the, serial->format);
}

void xs_serial_set_format(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	if (!serial) return;
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("unimplemented");
	serial->format = format;
}

void xs_serial_read(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	int count;

	if (!serial)
		xsUnknownError("closed");

	count = uart_ll_get_rxfifo_len(serial->uart_reg);
	if (0 == count)
		return;

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		uart_ll_read_rxfifo(serial->uart_reg, &byte, 1);

		xsmcSetInteger(xsResult, byte);
	}
	else {
		uint8_t *buffer;
		int requested = xsmcArgc ? xsmcToInteger(xsArg(0)) : count;
		if (requested > count)
			requested = count;

		xsmcSetArrayBuffer(xsResult, NULL, requested);
		buffer = xsmcToArrayBuffer(xsResult);

		uart_ll_read_rxfifo(serial->uart_reg, buffer, requested);
	}

	if (serial->onReadable)
		uart_enable_rx_intr(serial->uart);
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	int count;

	if (!serial)
		xsUnknownError("closed");

	count = uart_ll_get_txfifo_len(serial->uart_reg);
	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		if (0 == count)
			xsUnknownError("output full");

		byte = (uint8_t)xsmcToInteger(xsArg(0));
		uart_ll_write_txfifo(serial->uart_reg, &byte, 1);
	}
	else {
		uint8_t *buffer;
		int requested = xsmcGetArrayBufferLength(xsArg(0));
		if (requested > count)
			xsUnknownError("output full");

		buffer = xsmcToArrayBuffer(xsArg(0));
		uart_ll_write_txfifo(serial->uart_reg, buffer, requested);
	}

	if (serial->onWritable) {
		uart_ll_set_txfifo_empty_thr(serial->uart_reg, 4);
		uart_enable_tx_intr(serial->uart, 1, 20);
	}
}

void ICACHE_RAM_ATTR serial_isr(void * arg)
{
	Serial serial = arg;
	uint8_t post = 0;
	int status = serial->uart_reg->int_st.val;

	if ((status & UART_INTR_TXFIFO_EMPTY) && serial->onWritable) {
		post |= (0 == serial->isWritable);
		serial->isWritable = 1;
		uart_ll_set_txfifo_empty_thr(serial->uart_reg, 0);
		uart_disable_tx_intr(serial->uart);
	}

	if ((status & (UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL)) && serial->onReadable) {
		post |= (0 == serial->isReadable);
		serial->isReadable = 1;
		uart_disable_rx_intr(serial->uart);
	}

	if (post)
		modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
}

void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	Serial serial = (Serial)refcon;
	int count;

	if (serial->isReadable) {
		serial->isReadable = 0;
		count = uart_ll_get_rxfifo_len(serial->uart_reg);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
		uart_enable_rx_intr(serial->uart);
	}

	if (serial->isWritable) {
		serial->isWritable = 0;
		count = uart_ll_get_txfifo_len(serial->uart_reg);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			xsEndHost(the);
		}
		uart_ll_set_txfifo_empty_thr(serial->uart_reg, 4);
		uart_enable_tx_intr(serial->uart, 1, 20);
	}
}

void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Serial serial = it;

	if (serial->onReadable)
		(*markRoot)(the, serial->onReadable);
	if (serial->onWritable)
		(*markRoot)(the, serial->onWritable);
}
