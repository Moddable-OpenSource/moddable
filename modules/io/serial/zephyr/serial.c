/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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
    To do:
        - fewer writable callbacks (only when clear or below high water mark)
        - don't post read / write when no readable/writable callbacks

    Warning: built with assistance from Claude.ai in September 2025. nearly 100 revisions later and 
                significant manual improvements, it works.
*/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.devicetree.h"
#include "mc.xs.h"		// for xsID_ values
#include "builtinCommon.h"

#include <zephyr/sys/ring_buffer.h>

#if kModZephyrSerialBusCount

#ifndef MODDEF_SERIAL_RX_BUFSIZE
	#define MODDEF_SERIAL_RX_BUFSIZE (256)
#endif
#ifndef MODDEF_SERIAL_TX_BUFSIZE
	#define MODDEF_SERIAL_TX_BUFSIZE (256)
#endif
#ifndef MODDEF_SERIAL_RX_CHUNKSIZE
	#define MODDEF_SERIAL_RX_CHUNKSIZE (16)
#endif
#ifndef MODDEF_SERIAL_TX_CHUNKSIZE
	#define MODDEF_SERIAL_TX_CHUNKSIZE (16)
#endif

enum {
	kSerialReadable = 1 << 0,
	kSerialWritable = 1 << 1,
	kSerialError = 1 << 2
};

typedef struct {
	xsMachine *the;
	xsSlot obj;
	xsSlot *onReadable;
	xsSlot *onWritable;
	xsSlot *onError;

	const struct device *device;
	struct ring_buf rxRingBuffer;
	struct ring_buf txRingBuffer;

	int lastError;
	uint8_t format;
	uint8_t flags;
	uint8_t txEnabled:1;
	uint8_t closed:1;
	int8_t useCount;

	uint8_t rxBuffer[MODDEF_SERIAL_RX_BUFSIZE];
	uint8_t txBuffer[MODDEF_SERIAL_TX_BUFSIZE];
} modSerialRecord, *modSerial;

// two dangers here: the instance is closed and a callback closing the instance
static void serialDeliver(void *the, void *refcon, uint8_t *, uint16_t)
{
	modSerial serial = (modSerial)refcon;

	unsigned int key = irq_lock();
	uint8_t flags = serial->flags;
	serial->flags = 0;
	irq_unlock(key);

	xsBeginHost(the);
		if ((flags & kSerialReadable) && !serial->closed) {
			int count = (int)ring_buf_size_get(&serial->rxRingBuffer);
			if (count) {
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			}
		}

		if ((flags & kSerialWritable) && !serial->closed)  {
			int count = (int)ring_buf_space_get(&serial->txRingBuffer);
			if (count) {
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			}
		}

		if ((flags & kSerialError) && !serial->closed) {
			xsmcSetInteger(xsResult, serial->lastError);
			xsCallFunction1(xsReference(serial->onError), serial->obj, xsResult);
		}
	xsEndHost(the);

	if (--serial->useCount <= 0) {
		c_free(serial);
		return;
	}
}

static void serial_uart_isr(const struct device *dev, void *user_data)
{
	modSerial serial = (modSerial)user_data;
    uint8_t flags = 0;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (serial->onError) {
            int error = uart_err_check(dev);
            if (error) {
                serial->lastError = error;
                flags |= kSerialError;
            }
        }

		if (uart_irq_rx_ready(dev)) {
			uint8_t rx_data[MODDEF_SERIAL_RX_CHUNKSIZE];
			int bytes_read;

			while ((bytes_read = uart_fifo_read(dev, rx_data, sizeof(rx_data))) > 0) {
				uint32_t written = ring_buf_put(&serial->rxRingBuffer, rx_data, bytes_read);
				if (serial->onReadable && (written > 0))
					flags |= kSerialReadable;

				if (serial->onError && (written < bytes_read)) {
					serial->lastError = -ENOMEM;		// buffer overflow
					flags |= kSerialError;
				}
			}
		}

		uint32_t space = uart_irq_tx_ready(dev);
		if (space > 0) {
			uint8_t tx_data[MODDEF_SERIAL_TX_CHUNKSIZE];
			uint32_t len = ring_buf_get(&serial->txRingBuffer, tx_data, 
						(space < MODDEF_SERIAL_TX_CHUNKSIZE) ? space : MODDEF_SERIAL_TX_CHUNKSIZE);
			if (len > 0) {
				uart_fifo_fill(dev, tx_data, len);

				if (serial->onWritable && (ring_buf_space_get(&serial->txRingBuffer) > 0))    //@@ this could fire less often
					flags |= kSerialWritable;
			}
			else {
				uart_irq_tx_disable(dev);
				serial->txEnabled = false;
				if (serial->onWritable)
					flags |= kSerialWritable;
			}
		}
	}

	uint8_t currentFlags = serial->flags;
	serial->flags |= flags;
	if (flags && !currentFlags) {
		serial->useCount++;
		modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
	}
}

void xs_serial_destructor(void *data)
{
	modSerial serial = (modSerial)data;
	if (!serial) return;

	if (serial->device) {
		uart_irq_rx_disable(serial->device);
		uart_irq_tx_disable(serial->device);
	}

	/* Don't free - useCount manages memory */
}

static void xs_serial_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	modSerial serial = (modSerial)it;

	if (serial->onReadable)
		(*markRoot)(the, serial->onReadable);
	if (serial->onWritable)
		(*markRoot)(the, serial->onWritable);
	if (serial->onError)
		(*markRoot)(the, serial->onError);
}

static const xsHostHooks xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

void xs_serial_constructor(xsMachine *the)
{
	modSerial serial = NULL;
	uint32_t baud;
	const struct modZephyrSerial *port = NULL;
	uint8_t format;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_port);
	port = modZephyrGetSerial(xsmcToString(xsVar(0)));
	if (!port)
		xsUnknownError("invalid port");
	if (!device_is_ready(port->device))
		xsUnknownError("uart not ready");

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	if ((baud <= 0) || (baud > 20000000))
		xsRangeError("invalid baud");

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatBuffer);
	if ((kIOFormatBuffer != format) && (kIOFormatNumber != format))
		xsUnknownError("unsupported format");

	struct uart_config uart_cfg = {
		.baudrate = baud,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	if (uart_configure(port->device, &uart_cfg) < 0)
		xsUnknownError("UART configure failed");

	serial = c_calloc(1, sizeof(modSerialRecord));
	if (!serial)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, serial);
	xsSetHostHooks(xsThis, &xsSerialHooks);

	serial->the = the;
	serial->obj = xsThis;
	serial->device = port->device;
	serial->format = format;
	serial->useCount = 1;

	ring_buf_init(&serial->rxRingBuffer, MODDEF_SERIAL_RX_BUFSIZE, serial->rxBuffer);
	ring_buf_init(&serial->txRingBuffer, MODDEF_SERIAL_TX_BUFSIZE, serial->txBuffer);

	uart_irq_callback_user_data_set(serial->device, serial_uart_isr, serial);

	uart_irq_rx_enable(serial->device);

	serial->onReadable = builtinGetCallback(the, xsID_onReadable);
	serial->onWritable = builtinGetCallback(the, xsID_onWritable);
	serial->onError = builtinGetCallback(the, xsID_onError);

	xsRemember(serial->obj);

	if (serial->onWritable) {
		serial->flags |= kSerialWritable;
		serial->useCount++;
		modMessagePostToMachine(serial->the, NULL, 0, serialDeliver, serial);
	}
}

void xs_serial_close(xsMachine *the)
{
	modSerial serial = xsmcGetHostData(xsThis);
	if (serial && xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks)) {
		serial->closed = true;
		xs_serial_destructor(serial);
		xsmcSetHostData(xsThis, NULL);
		xsSetHostDestructor(xsThis, NULL);

		xsForget(serial->obj);

		if (--serial->useCount <= 0)
			c_free(serial);
	}
}

void xs_serial_read(xsMachine *the)
{
	modSerial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	void *buffer;
	xsUnsignedValue buffer_length;
	int requested, available;

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		if (0 == ring_buf_get(&serial->rxRingBuffer, &byte, 1))
			return;

		xsmcSetInteger(xsResult, byte);
		return;
	}

	available = (int)ring_buf_size_get(&serial->rxRingBuffer);
	if (0 == available)
		return;

	if (0 == xsmcArgc)
		requested = available;
	else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsmcGetBufferWritable(xsArg(0), &buffer, &buffer_length);
		requested = buffer_length;
	}
	else
		requested = xsmcToInteger(xsArg(0));

	uint32_t readCount = (requested < available) ? requested : available;
	if (xsmcArgc && (xsReferenceType == xsmcTypeOf(xsArg(0))))
		xsmcSetInteger(xsResult, readCount);
	else {
		xsmcSetArrayBuffer(xsResult, NULL, readCount);
		buffer = xsmcToArrayBuffer(xsResult);
	}

	ring_buf_get(&serial->rxRingBuffer, buffer, readCount);
}

void xs_serial_write(xsMachine *the)
{
	modSerial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	void *buffer;
	xsUnsignedValue length;
	uint8_t byte;

	if (kIOFormatNumber == serial->format) {
		byte = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &byte;
		length = 1;
	}
	else
		xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (ring_buf_space_get(&serial->txRingBuffer) < length)
		xsUnknownError("output full");

	ring_buf_put(&serial->txRingBuffer, buffer, length);

	unsigned int key = irq_lock();
	if (!serial->txEnabled) {
		uart_irq_tx_enable(serial->device);
		serial->txEnabled = true;
	}
	irq_unlock(key);
}

void xs_serial_get_format(xsMachine *the)
{
	modSerial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	builtinGetFormat(the, serial->format);
}

void xs_serial_set_format(xsMachine *the)
{
	modSerial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	uint8_t format = builtinSetFormat(the);

	if ((kIOFormatBuffer != format) && (kIOFormatNumber != format))
		xsRangeError("unsupported format");

	serial->format = format;
}

#else // !kModZephyrSerialBusCount

void xs_serial_constructor(xsMachine *the)
{
	xsUnknownError("no serial");
}

void xs_serial_destructor(void *) {}
void xs_serial_close(xsMachine *the) {}
void xs_serial_read(xsMachine *the) {}
void xs_serial_write(xsMachine *the) {}
void xs_serial_get_format(xsMachine *the) {}
void xs_serial_set_format(xsMachine *the) {}

#endif
