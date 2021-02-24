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

#include "xsHost.h"

#include "builtinCommon.h"

#include "nrf.h"
#include "nrf_libuarte_async.h"
#include "queue.h"
#include "sdk_config.h"

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

// transmit buffer needs to be bigger to avoid overflow on echo. Why?
#define kReceiveBytes (256)
#define kTransmitBytes (kReceiveBytes * 3)

struct SerialRecord {
	xsSlot		obj;
	uint8_t		format;
	nrf_libuarte_async_t	uart;
	int32_t		transmitPin;
	int32_t		receivePin;

	uint8_t		receiveTriggered;
	uint16_t	receiveCount;
	uint8_t		receive[kReceiveBytes];

	uint8_t		transmit[kTransmitBytes];
	uint16_t	transmitPosition;
	uint16_t	transmitSent;
	uint8_t		transmitTriggered;

	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
};

static void uart_handler(void *context, nrf_libuarte_async_evt_t * p_evt);
static void serialDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

#define kInvalidPin (-1)

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

#define SER_UARTE_NAME	gUART
#define SER_UARTE_IDX	1
#define SER_TIMER0_IDX	1
#define SER_RTC1_IDX	0
#define SER_TIMER1_IDX	NRF_LIBUARTE_PERIPHERAL_NOT_USED
#define SER_RX_BUF_SIZE	255
#define SER_RX_BUF_CNT	3

NRF_LIBUARTE_ASYNC_DEFINE(SER_UARTE_NAME, SER_UARTE_IDX, SER_TIMER0_IDX, SER_RTC1_IDX, SER_TIMER1_IDX, SER_RX_BUF_SIZE, SER_RX_BUF_CNT);

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t hasReadable, hasWritable, format;
	int transmitPin = kInvalidPin, receivePin = kInvalidPin;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_transmit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_transmit);
		transmitPin = xsmcToInteger(xsVar(0));
		if (!builtinIsPinFree(transmitPin))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_receive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_receive);
		receivePin = xsmcToInteger(xsVar(0));
		if (!builtinIsPinFree(receivePin))
			xsUnknownError("in use");
	}
	else if (kInvalidPin == transmitPin)
		xsUnknownError("invalid");

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	switch (baud) {
		case 1200:
			baud = NRF_UARTE_BAUDRATE_1200;
			break;
		case 2400:
			baud = NRF_UARTE_BAUDRATE_2400;
			break;
		case 4800:
			baud = NRF_UARTE_BAUDRATE_4800;
			break;
		case 9600:
			baud = NRF_UARTE_BAUDRATE_9600;
			break;
		case 14400:
			baud = NRF_UARTE_BAUDRATE_14400;
			break;
		case 19200:
			baud = NRF_UARTE_BAUDRATE_19200;
			break;
		case 28800:
			baud = NRF_UARTE_BAUDRATE_28800;
			break;
		case 38400:
			baud = NRF_UARTE_BAUDRATE_38400;
			break;
		case 57600:
			baud = NRF_UARTE_BAUDRATE_57600;
			break;
		case 76800:
			baud = NRF_UARTE_BAUDRATE_76800;
			break;
		case 115200:
			baud = NRF_UARTE_BAUDRATE_115200;
			break;
		case 230400:
			baud = NRF_UARTE_BAUDRATE_230400;
			break;
		case 250000:
			baud = NRF_UARTE_BAUDRATE_250000;
			break;
		case 460800:
			baud = NRF_UARTE_BAUDRATE_460800;
			break;
		case 921600:
			baud = NRF_UARTE_BAUDRATE_921600;
			break;
		case 1000000:
			baud = NRF_UARTE_BAUDRATE_1000000;
			break;
		default:
			xsRangeError("invalid baud");
			break;
	}

	hasReadable = (kInvalidPin != receivePin) && builtinHasCallback(the, xsID_onReadable);
	hasWritable = (kInvalidPin != transmitPin) && builtinHasCallback(the, xsID_onWritable);

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	serial = c_calloc(1, sizeof(SerialRecord));
	if (!serial)
		xsRangeError("no memory");

	nrf_libuarte_async_config_t uartConfig = {
		.tx_pin = transmitPin,
		.rx_pin = receivePin,
		.baudrate = baud,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.timeout_us = 100,
		.int_prio = 5
	};

	serial->uart = gUART;

	ret_code_t ret = nrf_libuarte_async_init(&serial->uart, &uartConfig, uart_handler, serial);
	if (ret) {
		c_free(serial);
		xsRangeError("init failed");
	}

	xsmcSetHostData(xsThis, serial);

	serial->the = the;
	serial->obj = xsThis;
	xsRemember(serial->obj);

	serial->transmitPin = transmitPin;
	serial->receivePin = receivePin;
	serial->format = format;

	if (hasReadable || hasWritable) {
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

		if (hasReadable) {
			builtinGetCallback(the, xsID_onReadable, &xsVar(0));
			serial->onReadable = xsToReference(xsVar(0));
		}

		if (hasWritable) {
			builtinGetCallback(the, xsID_onWritable, &xsVar(0));
			serial->onWritable = xsToReference(xsVar(0));
		}
	}

	nrf_libuarte_async_enable(&serial->uart);

	if (kInvalidPin != transmitPin)
		builtinUsePin(transmitPin);
	if (kInvalidPin != receivePin)
		builtinUsePin(receivePin);

	if (serial->onWritable) {
		serial->transmitTriggered = true;
		modMessagePostToMachine(serial->the, NULL, 0, serialDeliver, serial);
	}
}

void xs_serial_destructor(void *data)
{
	Serial serial = data;
	if (!serial) return;

	nrf_libuarte_async_uninit(&serial->uart);

	if (kInvalidPin != serial->transmitPin)
		builtinFreePin(serial->transmitPin);
	if (kInvalidPin != serial->receivePin)
		builtinFreePin(serial->receivePin);

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
	builtinGetFormat(the, serial->format);
}

void xs_serial_set_format(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("unimplemented");
	serial->format = format;
}

void xs_serial_read(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	int receiveCount;

	if (!serial)
		xsUnknownError("closed");

	receiveCount = serial->receiveCount;
	if (0 == receiveCount)
		return;

	if (kIOFormatNumber == serial->format) {
		xsmcSetInteger(xsResult, serial->receive[0]);

		builtinCriticalSectionBegin();
			serial->receiveCount -= 1;
			if (serial->receiveCount)
				c_memmove(serial->receive, serial->receive + 1, serial->receiveCount);
		builtinCriticalSectionEnd();
	}
	else {
		int requested = xsmcArgc ? xsmcToInteger(xsArg(0)) : receiveCount;
		if (requested > receiveCount)
			requested = receiveCount;

		xsmcSetArrayBuffer(xsResult, serial->receive, requested);
		builtinCriticalSectionBegin();
			serial->receiveCount -= requested;
			if (serial->receiveCount)
				c_memmove(serial->receive, serial->receive + requested, serial->receiveCount);
		builtinCriticalSectionEnd();
	}
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	int available;
	uint8_t *src;
	uint8_t b;

	if (!serial)
		xsUnknownError("closed");

	available = kTransmitBytes - serial->transmitPosition;

	if (kIOFormatNumber == serial->format) {
		if (0 == available)
			xsUnknownError("output full");

		b = (uint8_t)xsmcToInteger(xsArg(0));
		src = &b;
		available = 1;
	}
	else {
		int requested = xsmcGetArrayBufferLength(xsArg(0));
		if (requested > available)
			xsUnknownError("output full");

		src = xsmcToArrayBuffer(xsArg(0));
		available = requested;
	}

	builtinCriticalSectionBegin();
		c_memmove(&serial->transmit[serial->transmitPosition], src, available);
		serial->transmitPosition += available;

		if (available == serial->transmitPosition)
			nrf_libuarte_async_tx(&serial->uart, serial->transmit, serial->transmitPosition);
	builtinCriticalSectionEnd();
}

static void serialDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Serial serial = refcon;

	if (serial->transmitTriggered) {
		serial->transmitTriggered = false;
		if (serial->onWritable && (0 == serial->transmitPosition)) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, kTransmitBytes);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}

	if (serial->receiveTriggered) {
		int32_t receiveCount;

		builtinCriticalSectionBegin();
			receiveCount = serial->receiveCount;
			serial->receiveTriggered = false;
		builtinCriticalSectionEnd();

		if (serial->onReadable && receiveCount) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, receiveCount);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}
}

void uart_handler(void *p_context, nrf_libuarte_async_evt_t *p_event)
{
	Serial serial = p_context;

	if (NRF_LIBUARTE_ASYNC_EVT_RX_DATA == p_event->type) {
		if (kReceiveBytes == serial->receiveCount)
			;	// overflow!
		else {
			size_t length = p_event->data.rxtx.length;
			if (length > (kReceiveBytes - serial->receiveCount))
				length = kReceiveBytes - serial->receiveCount;
			c_memmove(&serial->receive[serial->receiveCount], p_event->data.rxtx.p_data, length);
			serial->receiveCount += length;
			if (serial->onReadable && !serial->receiveTriggered) {
				if (!serial->transmitTriggered) {
					serial->receiveTriggered = true;
					modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
				}
			}
		}
		
		nrf_libuarte_async_rx_free(&serial->uart, p_event->data.rxtx.p_data, p_event->data.rxtx.length);
	}
	else if (NRF_LIBUARTE_ASYNC_EVT_TX_DONE == p_event->type) {
		serial->transmitSent += p_event->data.rxtx.length;

		if (serial->transmitSent == serial->transmitPosition) {
			serial->transmitSent = serial->transmitPosition = 0;
			if (serial->onWritable && !serial->receiveTriggered) {
				serial->transmitTriggered = true;
				modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
			}
		}
		else {
			c_memmove(serial->transmit, &serial->transmit[serial->transmitSent], serial->transmitPosition - serial->transmitSent);
			serial->transmitPosition -= serial->transmitSent;
			serial->transmitSent = 0;
			nrf_libuarte_async_tx(&serial->uart, serial->transmit, serial->transmitPosition);
		}
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
