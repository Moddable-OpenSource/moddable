/*
 * Copyright (c) 2019-2020  Moddable Tech, Inc.
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
*/


#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#ifdef __ets__
	#include "uart.h"
	#include "esp8266_peri.h"


	#define UART_NR (UART0)
	#define kTXPin (1)
	#define kRXPin (3)

	#define getBytesReadable() ((USS(UART_NR) >> USRXC) & 0x7F)
	#define getBytesWritable() (128 - ((USS(UART_NR) >> USTXC) & 0x7F))

	static void ICACHE_RAM_ATTR serial_isr(void * arg);
#endif

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

struct SerialRecord {
	xsSlot		obj;
	uint8_t		hasReadable;
	uint8_t		hasWritable;
	uint8_t		isReadable;
	uint8_t		isWritable;
	uint8_t		format;
	// fields after here only allocated if callbacks used
	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
};

static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength);

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t format;
	xsSlot *onReadable, *onWritable;

	if (!builtinIsPinFree(kTXPin) || !builtinIsPinFree(kRXPin))
		xsUnknownError("in use");

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	if ((baud < 0) || (baud > 20000000))
		xsRangeError("invalid baud");

	onReadable = builtinGetCallback(the, xsID_onReadable);
	onWritable = builtinGetCallback(the, xsID_onWritable);

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	serial = c_malloc((onReadable || onWritable) ? sizeof(SerialRecord) : offsetof(SerialRecord, the));
	if (!serial)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, serial);

	serial->obj = xsThis;
	xsRemember(serial->obj);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

	serial->format = format;
	serial->hasReadable = onReadable ? 1 : 0;
	serial->hasWritable = onWritable ? 1 : 0;
	if (onReadable || onWritable) {
		serial->the = the;

		if (onReadable) {
			serial->isReadable = 0;
			serial->onReadable = onReadable; 
		}

		if (onWritable) {
			serial->isWritable = 0;
			serial->onWritable = onWritable; 
		}
	}

	ETS_UART_INTR_DISABLE();

//	pinMode(3, SPECIAL);
	GPC(kRXPin) = (GPC(kRXPin) & (0xF << GPCI)); // SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
	GPEC = (1 << kRXPin); // disable
	GPF(kRXPin) = GPFFS(GPFFS_BUS(kRXPin)); // Set mode to BUS (RX0, TX0, TX1, SPI, HSPI or CLK depending in the pin)
	GPF(kRXPin) |= (1 << GPFPU); // enable pullup on RX

//	pinMode(1, FUNCTION_0);
	GPC(kTXPin) = (GPC(kTXPin) & (0xF << GPCI)); // SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
	GPEC = (1 << kTXPin); // disable
	GPF(kTXPin) = GPFFS(0);

	IOSWAP &= ~(1 << IOSWAPU0);

	USC0(UART_NR) = UART_8N1;
	USD(UART_NR) = (ESP8266_CLOCK / baud);

	#define RXTX_RESET_MASK ((1 << UCRXRST) | (1 << UCTXRST))
	USC0(UART_NR) |= RXTX_RESET_MASK;
	USC0(UART_NR) &= ~RXTX_RESET_MASK;

	// configure interrupts
	if (onReadable || onWritable) {
		uint32_t usie = 0, usc1 = 0;
		if (onReadable) {
			usc1 = (120 << UCFFT) | (0x02 << UCTOT) | (1 << UCTOE );		// generate interrupt after 120 bytes received or 2 character time recieve timeout
			usie = (1 << UIFF) | (1 << UITO);
		}
		if (onWritable) {
			usc1 |= (16 << UCFET);		// 16 characters remain to transmit
			usie |= (1 << UIFE);
		}
		USC1(UART_NR) = usc1;
		USIC(UART_NR) = 0xffff;
		USIE(UART_NR) = usie;
		ETS_UART_INTR_ATTACH(serial_isr,  (void *)serial);
	}

	ETS_UART_INTR_ENABLE();

	if (onWritable) {
		serial->isWritable = 1;
		modMessagePostToMachineFromPool(serial->the, serialDeliver, serial);
	}

	builtinUsePin(kTXPin);
	builtinUsePin(kRXPin);
}

void xs_serial_destructor(void *data)
{
	Serial serial = data;
	if (!serial) return;

	ETS_UART_INTR_DISABLE();
	USC1(UART_NR) = 0;
	USIC(UART_NR) = 0xffff;
	USIE(UART_NR) = 0;
	ETS_UART_INTR_ATTACH(NULL, NULL);

	c_free(serial);

	builtinFreePin(kTXPin);
	builtinFreePin(kRXPin);
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

	available = getBytesReadable();
	if (0 == available)
		return;

	if (kIOFormatNumber == serial->format)
		xsmcSetInteger(xsResult, USF(UART_NR));
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
			if (requested > available)
				requested = available;
			allocate = 0;
			xsmcSetInteger(xsResult, requested);
		}
		else
			requested = xsmcToInteger(xsArg(0));

		if (requested <= 0) 
			xsUnknownError("invalid");

		if (allocate)
			buffer = xsmcSetArrayBuffer(xsResult, NULL, requested);

		while (requested--)
			*buffer++ = USF(UART_NR);
	}
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int count = getBytesWritable();

	if (kIOFormatNumber == serial->format) {
		if (0 == count)
			xsUnknownError("output full");

		USF(UART_NR) = xsmcToInteger(xsArg(0));
	}
	else {
		uint8_t *buffer;
		xsUnsignedValue requested;

		xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &requested);
		if (requested > count)
			xsUnknownError("output full");

		while (requested--)
			USF(UART_NR) = *buffer++;
	}

	USIC(UART_NR) = (1 << UIFE);			// clear pending notification
	USIE(UART_NR) |= (1 << UIFE);			// enable TX FIFO empty interrupts
}

void ICACHE_RAM_ATTR serial_isr(void * arg)
{
	Serial serial = (Serial)arg;
	uint8_t triggered = serial->isReadable | serial->isWritable;
	uint32_t status = USIS(UART_NR);

	if (status & ((1 << UIFF) | (1 << UITO)))
		serial->isReadable = serial->hasReadable;

	if (status & (1 << UIFE)) {
		serial->isWritable = serial->hasWritable;
		USIE(UART_NR) &= ~(1 << UIFE);		// disable TX FIFO empty interrupts until next write
	}

	USIC(UART_NR) = status;

	if ((serial->isReadable || serial->isWritable) && !triggered) {
		ETS_UART_INTR_DISABLE();
		modMessagePostToMachineFromPool(serial->the, serialDeliver, serial);
	}
}

void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	Serial serial = (Serial)refcon;
	int count;

	if (serial->isReadable) {
		serial->isReadable = 0;
		count = getBytesReadable();
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}

	if (serial->isWritable) {
		serial->isWritable = 0;
		count = getBytesWritable();
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}

	ETS_UART_INTR_ENABLE();
}

void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Serial serial = it;

	if (serial->hasReadable)
		(*markRoot)(the, serial->onReadable);
	if (serial->hasWritable)
		(*markRoot)(the, serial->onWritable);
}
