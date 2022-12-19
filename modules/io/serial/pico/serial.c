/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
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
#include "serial_fifo.h"

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

#define FIFO_SIZE	128	// 1024

struct SerialRecord {
	xsSlot		obj;
	uint8_t		isReadable;
	uint8_t		isWritable;
	uint8_t		format;
	uint8_t		useCount;
	uint8_t		txInterruptEnabled;
	uint8_t		rxInterruptEnabled;
	uint8_t		hasOnReadableOrWritable;
	uint8_t		msgOut;
	uart_inst_t	*uart;
	int			uart_irq;
	uint32_t	transmit;
	uint32_t	receive;
	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
	fifo_t		tx_fifo;
	uint8_t		*tx_fifo_buffer;
	fifo_t		rx_fifo;
	uint8_t		*rx_fifo_buffer;
};

static void uart_irq();
static void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

#define kTransmitTreshold (10)		// could be build-time or runtime configurable
#define UART_PIN_NO_CHANGE	(0xff)

static const uint8_t uart_chan_0_tx[] = { 0, 12, 16, 28, 0xff };
static const uint8_t uart_chan_0_rx[] = { 1, 13, 17, 29, 0xff };
static const uint8_t uart_chan_1_tx[] = { 4, 8, 20, 24, 0xff };
static const uint8_t uart_chan_1_rx[] = { 5, 9, 21, 25, 0xff };
static Serial uart_in_use[2] = {0, 0};

int uart_for_pin(int transmit, int receive)
{
	int i;
	if (UART_PIN_NO_CHANGE != transmit) {
		for (i=0; uart_chan_0_tx[i] != 0xff; i++)
			if (uart_chan_0_tx[i] == transmit)
				return 0;
		for (i=0; uart_chan_1_tx[i] != 0xff; i++)
			if (uart_chan_1_tx[i] == transmit)
				return 1;
	}
	else if (UART_PIN_NO_CHANGE != receive) {
		for (i=0; uart_chan_0_tx[i] != 0xff; i++)
			if (uart_chan_0_rx[i] == receive)
				return 0;
		for (i=0; uart_chan_1_tx[i] != 0xff; i++)
			if (uart_chan_1_rx[i] == receive)
				return 1;
	}
	return -1;
}

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t hasReadable, hasWritable, format;
	xsSlot *onReadable, *onWritable;
	int err;
	int uart_id = 0;
	uint32_t transmit = UART_PIN_NO_CHANGE, receive = UART_PIN_NO_CHANGE;
	uart_inst_t *uart;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_transmit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_transmit);
		transmit = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(transmit))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_receive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_receive);
		receive = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(receive))
			xsUnknownError("in use");
	}
	else if (UART_PIN_NO_CHANGE == transmit)
		xsUnknownError("invalid");

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	if ((baud <= 0) || (baud > 20000000))
		xsRangeError("invalid baud");

	onReadable = builtinGetCallback(the, xsID_onReadable);
	onWritable = builtinGetCallback(the, xsID_onWritable);

	hasReadable = (UART_PIN_NO_CHANGE != receive) && onReadable;
	hasWritable = (UART_PIN_NO_CHANGE != transmit) && onWritable;

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	// check if port is in use
	uart_id = uart_for_pin(transmit, receive);
	if (-1 == uart_id)
		xsUnknownError("invalid");

	if (uart_in_use[uart_id])
		xsRangeError("port in use");

	uart = uart_id ? (uart_inst_t*)uart1_hw : (uart_inst_t*)uart0_hw;

	uart_init(uart, baud);
	uart_set_format(uart, 8, 1, UART_PARITY_NONE);

	serial = c_calloc((hasReadable || hasWritable) ? sizeof(SerialRecord) : offsetof(SerialRecord, the), 1);
	if (!serial)
		xsRangeError("no memory");

	uart_in_use[uart_id] = serial;
	serial->uart = uart;

	xsmcSetHostData(xsThis, serial);

	serial->obj = xsThis;

	serial->transmit = transmit;
	serial->receive = receive;
	serial->format = format;
	serial->isReadable = 0;
	serial->isWritable = 0;
	serial->useCount = 1;
	serial->hasOnReadableOrWritable = hasReadable || hasWritable;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

	gpio_set_function(transmit, GPIO_FUNC_UART);
	gpio_set_function(receive, GPIO_FUNC_UART);

	serial->rx_fifo_buffer = c_malloc(FIFO_SIZE);
	fifo_init(&serial->rx_fifo, serial->rx_fifo_buffer, FIFO_SIZE);
	serial->tx_fifo_buffer = c_malloc(FIFO_SIZE);
	fifo_init(&serial->tx_fifo, serial->tx_fifo_buffer, FIFO_SIZE);

	if (serial->hasOnReadableOrWritable) {
		serial->the = the;

		if (hasReadable)
			serial->onReadable = onReadable;
		else
			serial->onReadable = NULL;

		if (hasWritable)
			serial->onWritable = onWritable;
		else
			serial->onWritable = NULL;
	}

	xsRemember(serial->obj);

	if (UART_PIN_NO_CHANGE != transmit)
		builtinUsePin(transmit);
	if (UART_PIN_NO_CHANGE != receive)
		builtinUsePin(receive);

	serial->uart_irq = uart_id ? UART1_IRQ : UART0_IRQ;	
	uart_set_fifo_enabled(uart, true);	//@@
	irq_set_exclusive_handler(serial->uart_irq, uart_irq);
	serial->rxInterruptEnabled = hasReadable;
	serial->txInterruptEnabled = hasWritable;
	irq_set_enabled(serial->uart_irq, true);
//	uart_set_irq_enables(uart, serial->rxInterruptEnabled, serial->txInterruptEnabled);

	uart_irq();
}

void xs_serial_destructor(void *data)
{
	Serial serial = data;
	if (!serial) return;

	uart_set_irq_enables(serial->uart, false, false);
	irq_remove_handler(serial->uart_irq, uart_irq);

	if (UART_PIN_NO_CHANGE != serial->transmit)
		builtinFreePin(serial->transmit);
	if (UART_PIN_NO_CHANGE != serial->receive)
		builtinFreePin(serial->receive);

//	if (0 == __atomic_sub_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST))
	if (0 == --serial->useCount)
		c_free(serial);
}

void xs_serial_close(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
modLog("serialClose");
	if (serial && xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks)) {
		xsForget(serial->obj);
		xs_serial_destructor(serial);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_serial_get_format(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	builtinGetFormat(the, serial->format);
}

void xs_serial_set_format(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("unimplemented");
	serial->format = format;
}

void xs_serial_read(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int available = fifo_length(&serial->rx_fifo);

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		if (0 != fifo_get(&serial->rx_fifo, &byte))
			xsUnknownError("no data");

		xsmcSetInteger(xsResult, byte);
	}
	else {
		uint8_t *buffer;
		int requested;
		xsUnsignedValue byteLength;
		uint8_t allocate = 1;
		
		if (0 == xsmcArgc)
			requested = available;
		else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
			xsResult = xsArg(0);
			xsmcGetBufferWritable(xsResult, (void **)&buffer, &byteLength);
			requested = (int)byteLength;
			allocate = 0;
		}
		else
			requested = xsmcToInteger(xsArg(0));

		if ((requested <= 0) || (requested > available)) 
			xsUnknownError("invalid");

		if (allocate)
			buffer = xsmcSetArrayBuffer(xsResult, NULL, requested);

		while (requested--) {
			if (0 != fifo_get(&serial->rx_fifo, buffer++))
				xsUnknownError("no data");
		}
	}

	serial->rxInterruptEnabled = (NULL != serial->onReadable);
	uart_irq();
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int count = FIFO_SIZE - fifo_length(&serial->tx_fifo);

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		if (0 == count)
			xsUnknownError("output full");

		byte = (uint8_t)xsmcToInteger(xsArg(0));
		if (0 != fifo_put(&serial->tx_fifo, byte))
			xsUnknownError("tx full");
	}
	else {
		uint8_t *buffer;
		xsUnsignedValue requested;

		xsmcGetBufferReadable(xsArg(0), &buffer, &requested);
		if (requested > count)
			xsUnknownError("output full");

		while (requested--) {
			if (0 != fifo_put(&serial->tx_fifo, *buffer++))
				xsUnknownError("tx full2");
		}
	}

	serial->txInterruptEnabled = (NULL != serial->onWritable);
	uart_irq();
}

void xs_serial_purge(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);

	fifo_flush(&serial->tx_fifo);
	fifo_flush(&serial->rx_fifo);
}

void uart_irq()
{
	Serial serial;
	uint8_t post = 0;
	int i;

	for (i=0; i<2; i++) {
		serial = uart_in_use[i];
		if (!serial)
			continue;
		uart_set_irq_enables(serial->uart, false, false);
		while (uart_is_writable(serial->uart) && (0 != fifo_length(&serial->tx_fifo))) {
			uint8_t c;
			if (0 == fifo_get(&serial->tx_fifo, &c))
				uart_write_blocking(serial->uart, &c, 1);
			else
				break;
		}

		serial->txInterruptEnabled = false;
		if (!fifo_full(&serial->tx_fifo)) {
			if (serial->onWritable) {
				post |= (0 == serial->isWritable);
				serial->isWritable = 1;
			}
		}
		if (0 != fifo_length(&serial->tx_fifo))
			serial->txInterruptEnabled = true;

		while (uart_is_readable(serial->uart) && !fifo_full(&serial->rx_fifo)) {
			uint8_t c = uart_getc(serial->uart);
			fifo_put(&serial->rx_fifo, c);
		}

		if (0 == (FIFO_SIZE - fifo_length(&serial->rx_fifo)))
			serial->rxInterruptEnabled = false;
		else if (serial->onReadable) {
			post |= (0 == serial->isReadable);
			serial->isReadable = 1;
		}

		if (post) {
//			__atomic_add_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST);
			serial->useCount += 1;
			if (!serial->msgOut) {
				serial->msgOut++;
				modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
			}
		}
	}

	for (i=0; i<2; i++) {
		serial = uart_in_use[i];
		if (!serial)
			continue;
		uart_set_irq_enables(serial->uart, serial->rxInterruptEnabled, serial->txInterruptEnabled);
	}
}

void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	Serial serial = (Serial)refcon;
	int count;

	if (0 == --serial->useCount) {
		c_free(serial);
		return;
	}

	serial->msgOut = 0;
	if (serial->isReadable && serial->onReadable) {
		serial->isReadable = 0;
		count = fifo_length(&serial->rx_fifo);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}

	if (serial->isWritable && serial->onWritable) {
		serial->isWritable = 0;
		count = FIFO_SIZE - fifo_length(&serial->tx_fifo);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}
}

void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Serial serial = it;

	if (!serial->hasOnReadableOrWritable)
		return;

	if (serial->onReadable)
		(*markRoot)(the, serial->onReadable);
	if (serial->onWritable)
		(*markRoot)(the, serial->onWritable);
}
