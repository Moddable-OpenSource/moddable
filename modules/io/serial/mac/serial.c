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

#include "xsAll.h"
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

/*
	to do:

		string;ascii doesn't ensure data is valid

*/

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

	xsVars(2);

	xsVar(0) = xsGet(xsArg(0), xsID_device);
	xsToStringBuffer(xsVar(0), device, sizeof(device));

	xsVar(0) = xsGet(xsArg(0), xsID_baud);
	baud = xsToInteger(xsVar(0));

	onReadable = xsGet(xsArg(0), xsID_onReadable);
	onWritable = xsGet(xsArg(0), xsID_onWritable);
	onError = xsGet(xsArg(0), xsID_onError);

	if (xsHas(xsArg(0), xsID_format)) {
		xsVar(0) = xsGet(xsArg(0), xsID_format);
		xsToStringBuffer(xsVar(0), format, sizeof(format));
	}
	else
		format[0] = 0;

	if (xsHas(xsArg(0), xsID_target)) {
		xsVar(0) = xsGet(xsArg(0), xsID_target);
		xsSet(xsThis, xsID_target, xsVar(0));
	}

	s = calloc(1, sizeof(xsSerialRecord));
	if (!s)
		xsUnknownError("no memory");
	xsSetHostData(xsThis, s);
	s->the = the;
	s->obj = xsThis;
	s->hasOnWritable = xsTest(onWritable);
	s->hasOnReadable = xsTest(onReadable);
	s->hasOnError = xsTest(onError);

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

		s->writable = modTimerAdd(0, 0, fxSerialWritable, &s, sizeof(s));
	}
}

void xs_serial_close(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;

	xsForget(s->obj);

	if (s->hasOnReadable)
		xsForget(s->onReadable);
	if (s->hasOnWritable)
		xsForget(s->onWritable);
	if (s->hasOnError)
		xsForget(s->onError);

	xs_serial_destructor(s);
	xsSetHostData(xsThis, NULL);
}

void xs_serial_read(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	int available = 0;
	int count;

	ioctl(s->fd, FIONREAD, &available);
	if (0 == available)
		return;

	if (xsToInteger(xsArgc)) {
		count = xsToInteger(xsArg(0));
		if (count > available)
			count = available;
	}
	else
		count = available;

	if (1 == s->bufferFormat) {
		xsResult = xsArrayBuffer(NULL, count);
		read(s->fd, xsToArrayBuffer(xsResult), count);
	}
	else if (2 == s->bufferFormat) {
		uint8_t byte;
		read(s->fd, &byte, 1);
		xsResult = xsInteger(byte);
	}
	else {
		xsResult = xsStringBuffer(NULL, count);
		read(s->fd, xsToString(xsResult), count);
	}
}

void xs_serial_write(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	int count;
	char *data;
	char byte;

	if (1 == s->bufferFormat) {
		count = xsGetArrayBufferLength(xsArg(0));
		data = xsToArrayBuffer(xsArg(0));
	}
	else if (2 == s->bufferFormat) {
		count = 1;
		byte = xsToInteger(xsArg(0));
		data = &byte;
	}
	else {
		data = xsToString(xsArg(0));
		count = strlen(data);
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
	xsSerial s = xsGetHostData(xsThis);
	int flags;

	ioctl(s->fd, TIOCMGET, &flags);

	if (xsHas(xsArg(0), xsID_RTS)) {
		if (xsTest(xsGet(xsArg(0), xsID_RTS)))
			flags |= TIOCM_RTS;
		else
			flags &= ~TIOCM_RTS;
	}

	if (xsHas(xsArg(0), xsID_DTR)) {
		if (xsTest(xsGet(xsArg(0), xsID_DTR)))
			flags |= TIOCM_DTR;
		else
			flags &= ~TIOCM_DTR;
	}

	ioctl(s->fd, TIOCMSET, &flags);
}

void xs_serial_format_get(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;
	if (2 == s->bufferFormat)
		xsResult = xsString("number");
	else if (1 == s->bufferFormat)
		xsResult = xsString("buffer");
	else
		xsResult = xsString("string;ascii");
}

void xs_serial_format_set(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;
	fxSerialSetFormat(the, s, xsToString(xsArg(0)));
}

void xs_serial_purge(xsMachine *the)
{
}

void fxSerialReadable(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	xsSerial s = context;

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
			ioctl(s->fd, FIONREAD, &count);
			xsCallFunction1(s->onReadable, s->obj, xsInteger(count));
		xsEndHost();
	}
}

void fxSerialWritable(modTimer timer, void *refcon, int refconSize)
{
	xsSerial s = *(xsSerial *)refcon;

	s->writable = NULL;

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
	else if (!strcmp(format, "string;ascii"))
		s->bufferFormat = 0;
	else
		xsUnknownError("invalid");
}
