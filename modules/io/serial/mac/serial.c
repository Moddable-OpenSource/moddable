/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
#include "mc.xs.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>

#include "modTimer.h"

#define kWriteBufferSize (1024)		// 1024 is the default on macOS (it seems)

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				onReadable;
	xsSlot				onWritable;
	xsSlot				onError;

	int					fd;
	CFSocketRef			serialSocket;
	CFRunLoopSourceRef	serialSource;

	modTimer			writable;
	modTimer			flush;
	uint8_t				bufferFormat;
	uint8_t				hasOnReadable;
	uint8_t				hasOnWritable;
	uint8_t				hasOnError;
	uint8_t				initialWritablePending;

	uint8_t				writeBuffer[kWriteBufferSize];
	uint8_t				*toWrite;
	uint32_t			bytesToWrite;
} xsSerialRecord, *xsSerial;

static void fxSerialReadable(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
static void fxSerialWritable(modTimer timer, void *refcon, int refconSize);
static void fxSerialFlush(modTimer timer, void *refcon, int refconSize);
static void fxSerialSetFormat(xsMachine *the, xsSerial s, char *format);

void xs_serial_destructor(void *data)
{
	xsSerial s = data;
	if (!s) return;

	if (s->writable)
		modTimerRemove(s->writable);

	if (s->flush)
		modTimerRemove(s->flush);

	if (s->serialSocket)
		CFSocketInvalidate(s->serialSocket);

	free(data);
}

void xs_serial_constructor(xsMachine *the)
{
	char device[128], format[64];
	speed_t baud;
	CFSocketContext context;
	int fd;
	xsSerial s;
	xsSlot onReadable, onWritable, onError;

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_device);
	xsmcToStringBuffer(xsVar(0), device, sizeof(device));

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));

	xsmcGet(onReadable, xsArg(0), xsID_onReadable);
	xsmcGet(onWritable, xsArg(0), xsID_onWritable);
	xsmcGet(onError, xsArg(0), xsID_onError);

	if (xsmcHas(xsArg(0), xsID_format)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_format);
		xsmcToStringBuffer(xsVar(0), format, sizeof(format));
	}
	else
		format[0] = 0;

	if (xsmcHas(xsArg(0), xsID_target)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_target);
		xsmcSet(xsThis, xsID_target, xsVar(0));
	}

	s = calloc(1, sizeof(xsSerialRecord));
	if (!s)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, s);
	s->the = the;
	s->obj = xsThis;
	s->hasOnWritable = xsmcTest(onWritable);
	s->hasOnReadable = xsmcTest(onReadable);
	s->hasOnError = xsmcTest(onError);

	if (format[0])
		fxSerialSetFormat(the, s, format);
	else
		s->bufferFormat = 1;

	fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (-1 == fd)
		xsUnknownError("can't open serial");

    if (-1 == ioctl(fd, TIOCEXCL)) {
		close(fd);
		xsUnknownError("Error setting TIOCEXCL");
    }

    if (-1 == ioctl(fd, IOSSIOSPEED, &baud)) {
		close(fd);
        xsUnknownError( "Error calling ioctl(..., IOSSIOSPEED, ...");
	}

	memset(&context, 0, sizeof(CFSocketContext));
	context.info = (void *)s;
	s->fd = fd;
	s->serialSocket = CFSocketCreateWithNative(kCFAllocatorDefault, fd, kCFSocketReadCallBack, fxSerialReadable, &context);
	s->serialSource = CFSocketCreateRunLoopSource(NULL, s->serialSocket, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), s->serialSource, kCFRunLoopCommonModes);

	xsRemember(s->obj);

	if (s->hasOnReadable) {
		s->onReadable = onReadable;
		xsRemember(s->onReadable);
	}
	if (s->hasOnError) {
		s->onError = onError;
		xsRemember(s->onError);
	}
	if (s->hasOnWritable) {
		s->onWritable = onWritable;
		xsRemember(s->onWritable);

		s->initialWritablePending = 1;
		s->writable = modTimerAdd(0, 0, fxSerialWritable, &s, sizeof(s));
	}
}

void xs_serial_close(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	if (!s) return;

	xsForget(s->obj);

	if (s->hasOnReadable)
		xsForget(s->onReadable);
	if (s->hasOnWritable)
		xsForget(s->onWritable);
	if (s->hasOnError)
		xsForget(s->onError);

	xs_serial_destructor(s);
	xsmcSetHostData(xsThis, NULL);
}

void xs_serial_read(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	int available = 0;

	ioctl(s->fd, FIONREAD, &available);
	if (0 == available)
		return;
		
	if (2 == s->bufferFormat) {
		uint8_t byte;
		read(s->fd, &byte, 1);
		xsResult = xsInteger(byte);
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
			if (requested > available)
				requested = available;
			allocate = 0;
			xsmcSetInteger(xsResult, requested);
		}
		else {
			requested = xsmcToInteger(xsArg(0));
			if (requested > available)
				requested = available;
		}
		if (requested <= 0) 
			xsUnknownError("invalid");
		if (allocate)
			buffer = xsmcSetArrayBuffer(xsResult, NULL, requested);
		read(s->fd, buffer, requested);
	}
}

void xs_serial_write(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	xsUnsignedValue count = 0;
	void *data;
	char byte;

	if (1 == s->bufferFormat) {
		xsmcGetBufferReadable(xsArg(0), &data, &count);
	}
	else if (2 == s->bufferFormat) {
		count = 1;
		byte = xsmcToInteger(xsArg(0));
		data = &byte;
	}
	if (!count)
		return;

	if ((count > kWriteBufferSize) || s->flush)
		xsUnknownError("would overflow");

	while (count) {
		int result = write(s->fd, data, count);
		if (result < 0) {
			if (EAGAIN != errno)
				xsUnknownError("write failed");

			c_memmove(s->writeBuffer, data, count);
			s->toWrite = s->writeBuffer;
			s->bytesToWrite = count;

			s->flush = modTimerAdd(0, 0, fxSerialFlush, &s, sizeof(s));
			return;
		}
		count -= result;
		data += result;
	}

	if (!s->writable && s->hasOnWritable)
		s->writable = modTimerAdd(0, 0, fxSerialWritable, &s, sizeof(s));
}

void xs_serial_set(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	int flags;

	xsmcVars(1);

	ioctl(s->fd, TIOCMGET, &flags);

	if (xsmcHas(xsArg(0), xsID_RTS)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_RTS);
		if (xsmcTest(xsVar(0)))
			flags |= TIOCM_RTS;
		else
			flags &= ~TIOCM_RTS;
	}

	if (xsmcHas(xsArg(0), xsID_DTR)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_DTR);
		if (xsmcTest(xsVar(0)))
			flags |= TIOCM_DTR;
		else
			flags &= ~TIOCM_DTR;
	}

	ioctl(s->fd, TIOCMSET, &flags);
}

void xs_serial_format_get(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	if (!s) return;
	if (2 == s->bufferFormat)
		xsResult = xsStringX("number");
	else if (1 == s->bufferFormat)
		xsResult = xsStringX("buffer");
}

void xs_serial_format_set(xsMachine *the)
{
	xsSerial s = xsmcGetHostData(xsThis);
	if (!s) return;
	fxSerialSetFormat(the, s, xsmcToString(xsArg(0)));
}

void xs_serial_purge(xsMachine *the)
{
}

void fxSerialReadable(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	xsSerial s = context;

	if (s->initialWritablePending)
		fxSerialWritable(s->writable, &s, sizeof(s));

	if (cbType & kCFSocketReadCallBack) {
		int count, err;

		err = ioctl(s->fd, FIONREAD, &count);
		if (err < 0) {
			xsBeginHost(s->the);
			if (s->hasOnError) {
				xsTry {
					xsCallFunction0(s->onError, s->obj);
				}
				xsCatch {
				}
			}
			xsCall0(s->obj, xsID("close"));
			xsEndHost();
			return;
		}

		if (!s->hasOnReadable)
			return;

		xsBeginHost(s->the);
			xsCallFunction1(s->onReadable, s->obj, xsInteger(count));
		xsEndHost();
	}
}

void fxSerialWritable(modTimer timer, void *refcon, int refconSize)
{
	xsSerial s = *(xsSerial *)refcon;

	s->writable = NULL;
	s->initialWritablePending = 0;

	xsBeginHost(s->the);
		xsCallFunction1(s->onWritable, s->obj, xsInteger(kWriteBufferSize));		// 1024 is the default on macOS?
	xsEndHost();
}

void fxSerialFlush(modTimer timer, void *refcon, int refconSize)
{
	xsSerial s = *(xsSerial *)refcon;

	s->flush = NULL;

	int result = write(s->fd, s->toWrite, s->bytesToWrite);
	if (result > 0) {
		s->toWrite += result;
		s->bytesToWrite -= result;
	}

	if (s->bytesToWrite)
		s->flush = modTimerAdd(0, 0, fxSerialFlush, &s, sizeof(s));
	else if (!s->writable && s->hasOnWritable)
		s->writable = modTimerAdd(0, 0, fxSerialWritable, &s, sizeof(s));
}

void fxSerialSetFormat(xsMachine *the, xsSerial s, char *format)
{
	if (!strcmp(format, "buffer"))
		s->bufferFormat = 1;
	else if (!strcmp(format, "number"))
		s->bufferFormat = 2;
	else
		xsUnknownError("invalid");
}
