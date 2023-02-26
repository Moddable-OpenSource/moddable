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

#include "xsHost.h"

#include "builtinCommon.h"

#include "nrfx_uarte.h"
#include "queue.h"
//#include "sdk_config.h"

int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon);

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

// transmit buffer needs to be bigger to avoid overflow on echo. Why?
#define kReceiveBytes (256)
#define kTransmitBytes (kReceiveBytes * 3)

struct SerialRecord {
	xsSlot		obj;
	uint8_t		format;
	int32_t		transmitPin;
	int32_t		receivePin;
	uint8_t		useCount;
	uint8_t		hasOnReadableOrWritable;

	uint8_t		receiveTriggered;
	uint16_t	receiveCount;
	uint8_t		receive[kReceiveBytes];
	uint8_t		rx_buffer[1];

	uint8_t		transmit[kTransmitBytes];
	uint16_t	transmitPosition;
	uint16_t	transmitSent;
	uint8_t		transmitTriggered;

	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
	uint8_t		postedMessage;
};

static void io_uart_handler(const nrfx_uarte_event_t *p_event, void *p_context);
static void serialDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

#define kInvalidPin (-1)

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

#if mxDebug
const nrfx_uarte_t gSerialUARTE = NRFX_UARTE_INSTANCE(1);
#else
const nrfx_uarte_t gSerialUARTE = NRFX_UARTE_INSTANCE(0);
#endif

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t hasReadable, hasWritable, format;
	xsSlot *onReadable, *onWritable;
	int transmitPin = kInvalidPin, receivePin = kInvalidPin, port = 0;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_transmit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_transmit);
		transmitPin = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(transmitPin))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_receive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_receive);
		receivePin = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(receivePin))
			xsUnknownError("in use");
	}
	else if (kInvalidPin == transmitPin)
		xsUnknownError("invalid");

	xsmcGet(xsVar(0), xsArg(0), xsID_port);
	port = xsmcToInteger(xsVar(0));
	if ((port < 0) || (port > 3))
		xsUnknownError("invalid port");

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

	onReadable = builtinGetCallback(the, xsID_onReadable);
	onWritable = builtinGetCallback(the, xsID_onWritable);

	hasReadable = (kInvalidPin != receivePin) && onReadable;
	hasWritable = (kInvalidPin != transmitPin) && onWritable;

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	serial = c_malloc((hasReadable || hasWritable) ?  sizeof(SerialRecord) : offsetof(SerialRecord, the));
	if (!serial)
		xsRangeError("no memory");

	nrfx_uarte_config_t uartConfig = {
		.pseltxd = transmitPin,
		.pselrxd = receivePin,
		.pselcts = NRF_UARTE_PSEL_DISCONNECTED,
		.pselrts = NRF_UARTE_PSEL_DISCONNECTED,
		.p_context = serial,
		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = baud,
		.interrupt_priority = 5
	};

	ret_code_t ret = nrfx_uarte_init(&gSerialUARTE, &uartConfig, io_uart_handler);
	if (ret) {
		c_free(serial);
		xsRangeError("init failed");
	}

	xsmcSetHostData(xsThis, serial);

	serial->obj = xsThis;

	serial->transmitPin = transmitPin;
	serial->receivePin = receivePin;
	serial->format = format;
	serial->useCount = 1;
	serial->hasOnReadableOrWritable = hasReadable || hasWritable;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

	if (serial->hasOnReadableOrWritable) {
		serial->the = the;

		if (hasReadable) {
			serial->onReadable = onReadable;
		}
		else
			serial->onReadable = NULL;

		if (hasWritable) {
			serial->onWritable = onWritable;
		}
		else
			serial->onWritable = NULL;
	}

	ret = nrfx_uarte_rx(&gSerialUARTE, serial->rx_buffer, 1);

	xsRemember(serial->obj);

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

	nrfx_uarte_uninit(&gSerialUARTE);

	if (kInvalidPin != serial->transmitPin)
		builtinFreePin(serial->transmitPin);
	if (kInvalidPin != serial->receivePin)
		builtinFreePin(serial->receivePin);

//	if (0 == __atomic_sub_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST))
		c_free(serial);
}

void xs_serial_close(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
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
	int available;

	if (!serial)
		xsUnknownError("closed");

	available = serial->receiveCount;
	if (0 == available)
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

		c_memcpy(buffer, serial->receive, requested);
	
		builtinCriticalSectionBegin();
			serial->receiveCount -= requested;
			if (serial->receiveCount)
				c_memmove(serial->receive, serial->receive + requested, serial->receiveCount);
		builtinCriticalSectionEnd();
	}
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int count;
	void *src;
	uint8_t byte;

	count = kTransmitBytes - serial->transmitPosition;

	if (kIOFormatNumber == serial->format) {
		if (0 == count)
			xsUnknownError("output full: count == 0");

		byte = (uint8_t)xsmcToInteger(xsArg(0));
		src = &byte;
		count = 1;
	}
	else {
		xsUnsignedValue requested;

		xsmcGetBufferReadable(xsArg(0), &src, &requested);
		if (requested > count)
			xsUnknownError("output full: requested > count");

		count = requested;
	}

	builtinCriticalSectionBegin();
		c_memmove(&serial->transmit[serial->transmitPosition], src, count);
		serial->transmitPosition += count;

		if (count == serial->transmitPosition)
			nrfx_uarte_tx(&gSerialUARTE, serial->transmit, serial->transmitPosition);
	builtinCriticalSectionEnd();
}

void xs_serial_purge(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	serial->transmitPosition = 0;
}

static void io_uart_handler(const nrfx_uarte_event_t *p_event, void *p_context)
{
	Serial serial = p_context;
	uint8_t post = 0;

	if (NRFX_UARTE_EVT_TX_DONE == p_event->type) {
		serial->transmitSent += p_event->data.rxtx.bytes;

		if (serial->transmitSent == serial->transmitPosition) {
			post = 1;
			serial->transmitSent = serial->transmitPosition = 0;
			serial->transmitTriggered = true;
		}
		else {
			c_memmove(serial->transmit, &serial->transmit[serial->transmitSent], serial->transmitPosition - serial->transmitSent);
			serial->transmitPosition -= serial->transmitSent;
			serial->transmitSent = 0;
			nrfx_uarte_tx(&gSerialUARTE, serial->transmit, serial->transmitPosition);
		}
	}
	else if (NRFX_UARTE_EVT_RX_DONE == p_event->type) {
		if (kReceiveBytes == serial->receiveCount)
			;	// overflow!
		else {
			size_t length = p_event->data.rxtx.bytes;
			if (length > (kReceiveBytes - serial->receiveCount))
				length = kReceiveBytes - serial->receiveCount;
			c_memmove(&serial->receive[serial->receiveCount], p_event->data.rxtx.p_data, length);
			serial->receiveCount += length;
			if (serial->onReadable && !serial->receiveTriggered) {
				post = 1;
				if (!serial->transmitTriggered) {
					serial->receiveTriggered = true;
//					modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
				}
			}
			nrfx_uarte_rx(&gSerialUARTE, serial->rx_buffer, 1);
		}

//		if (post) {
		if (post && !serial->postedMessage) {
			serial->postedMessage = 1;
//			__atomic_add_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST);
			modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
		}
	}
}

void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	Serial serial = (Serial)refcon;
	int count;
	uint8_t post = 0;

	serial->postedMessage = 0;
//	if (0 == __atomic_sub_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST)) {
//		c_free(serial);
//		return;
//	}

	if (serial->receiveTriggered) {
		builtinCriticalSectionBegin();
			count = serial->receiveCount;
			serial->receiveTriggered = false;
		builtinCriticalSectionEnd();

		if (serial->onReadable && count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}

	if (serial->transmitTriggered) {
		serial->transmitTriggered = false;
		if (serial->onWritable && (0 == serial->transmitPosition)) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, kTransmitBytes);
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

