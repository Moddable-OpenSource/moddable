/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFSocket.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <net/if_types.h>
#include <net/if_dl.h>

#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#define kBufferSize (1536)
enum {
	kTCPWritable = 1 << 0,
	kTCPReadable = 1 << 1,
	kTCPError = 1 << 2
};

struct TCPRecord {
    int					skt;
	xsSlot				obj;
	uint8_t				triggerable;
	uint8_t				triggered;
	int8_t				useCount;
	uint8_t				done;
	uint8_t				format;
	uint8_t				error;
	int16_t				port;
	CFSocketRef			cfSkt;
	CFRunLoopSourceRef	cfRunLoopSource;
	CFRunLoopTimerRef 	cfTriggeredTimer;
	xsMachine			*the;
	xsSlot				*onReadable;
	xsSlot				*onWritable;
	xsSlot				*onError;

	uint32_t			bytesWritable;
	uint32_t			bytesReadable;
	uint32_t			readPosition;
	uint8_t				readBuf[kBufferSize];
	
	struct TCPRecord	**handle;
};
typedef struct TCPRecord TCPRecord;
typedef struct TCPRecord *TCP;

static void tcpHold(TCP tcp);
static void tcpRelease(TCP tcp);
static void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void doClose(xsMachine *the, xsSlot *instance);
static void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info);
static void socketCallback(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef addr, const void *data, void *info);
static void tcpTrigger(TCP tcp, uint8_t trigger);

static const xsHostHooks xsTCPHooks = {
	xs_tcp_destructor,
	xs_tcp_mark,
	NULL
};

void xs_tcp_constructor(xsMachine *the)
{
	TCP tcp;
	uint8_t create = xsmcArgc > 0;
	uint8_t triggerable = 0, triggered = 0, nodelay = 0, format = kIOFormatBuffer;
	xsSlot *onReadable, *onWritable, *onError;
	int port = 0;

	xsmcVars(1);

	tcp = c_calloc(1, sizeof(TCPRecord));
	if (!tcp)
		xsRangeError("no memory");
	tcp->skt = -1;

	xsmcSetHostData(xsThis, tcp);

	if (create) {
		format = builtinInitializeFormat(the, format);
		if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
			xsRangeError("unimplemented");

		onReadable = builtinGetCallback(the, xsID_onReadable);
		if (onReadable)
			triggerable |= kTCPReadable;

		onWritable = builtinGetCallback(the, xsID_onWritable);
		if (onWritable)
			triggerable |= kTCPWritable;

		onError = builtinGetCallback(the, xsID_onError);
		if (onError)
			triggerable |= kTCPError;

		if (xsmcHas(xsArg(0), xsID_nodelay)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_nodelay);
			nodelay = xsmcToBoolean(xsVar(0)) ? 2 : 1;
		}

		if (xsmcHas(xsArg(0), xsID_from)) {
			TCP from;

			xsmcGet(xsVar(0), xsArg(0), xsID_from);
			from = xsmcGetHostDataValidate(xsVar(0), (void *)&xsTCPHooks);
			if (!from->skt)
				xsUnknownError("invalid from");

			triggered = from->triggered;
			tcp->cfRunLoopSource = from->cfRunLoopSource;
			tcp->cfSkt = from->cfSkt;
			tcp->skt = from->skt;
			tcp->port = from->port;
			tcp->bytesWritable = from->bytesWritable;
			tcp->bytesReadable = from->bytesReadable;
			tcp->readPosition = from->readPosition;
			memmove(tcp->readBuf, from->readBuf, sizeof(from->readBuf)); 
			tcp->handle = from->handle;
			*(tcp->handle) = tcp;
			tcp->error = from->error;

			from->cfRunLoopSource = NULL;
			from->cfSkt = NULL;
			from->skt = -1;
			from->triggered = 0;
			from->handle = NULL;
			doClose(the, &xsVar(0));

			if (tcp->bytesWritable)
				triggered |= kTCPWritable;
			if (tcp->bytesReadable)
				triggered |= kTCPReadable;
			if (tcp->error)
				triggered |= kTCPError;
		}
		else {
			char addrStr[32];

			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			xsmcToStringBuffer(xsVar(0), addrStr, sizeof(addrStr));

			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = xsmcToInteger(xsVar(0));
			if ((port < 0) || (port > 65535))
				xsRangeError("invalid port");

			tcp->handle = malloc(sizeof(TCP));
			if (!tcp->handle)
				xsRangeError("no memory");
			*(tcp->handle) = tcp;

			CFSocketContext socketCtxt = {0, tcp->handle, NULL, NULL, NULL};
			tcp->cfSkt = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketConnectCallBack | kCFSocketReadCallBack | kCFSocketWriteCallBack, socketCallback, &socketCtxt);
			tcp->skt = CFSocketGetNative(tcp->cfSkt);
			if (tcp->skt < 0) {
				CFSocketInvalidate(tcp->cfSkt);
				CFRelease(tcp->cfSkt);
				xsUnknownError("no socket");
			}

			int set = 1;
			setsockopt(tcp->skt, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

			fcntl(tcp->skt, F_SETFL, O_NONBLOCK | fcntl(tcp->skt, F_GETFL, 0));

			tcp->cfRunLoopSource = CFSocketCreateRunLoopSource(NULL, tcp->cfSkt, 0);
			CFRunLoopAddSource(CFRunLoopGetCurrent(), tcp->cfRunLoopSource, kCFRunLoopCommonModes);

			char *str;
			CFStringRef host;
			CFHostRef cfHost;

			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			str = xsmcToString(xsVar(0));
			host = CFStringCreateWithCString(NULL, (const char *)str, kCFStringEncodingUTF8);
			cfHost = CFHostCreateWithName(kCFAllocatorDefault, host);
			CFRelease(host);

			CFHostClientContext context;
			context.version = 0;
			context.retain = nil;
			context.release = nil;
			context.copyDescription = nil;
			context.info = tcp;

			CFHostSetClient(cfHost, resolved, &context);
			CFHostScheduleWithRunLoop(cfHost, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

			CFStreamError streamErr;
			if (!CFHostStartInfoResolution(cfHost, kCFHostAddresses, &streamErr))
				xsUnknownError("cannot resolve host address");
		}
	}

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsTCPHooks);
	tcp->the = the;
	tcp->obj = xsThis;
	xsRemember(tcp->obj);
	tcp->format = format;
	tcp->port = port;
	tcpHold(tcp);

	if (!create)
		return;

	if (nodelay) {
		int value = (2 == nodelay) ? 1 : 0;
		setsockopt(tcp->skt, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
	}

	builtinInitializeTarget(the);

	tcp->triggerable = triggerable;
	tcp->onReadable = onReadable;
	tcp->onWritable = onWritable;
	tcp->onError = onError;

	tcpTrigger(tcp, triggered);
}

void xs_tcp_destructor(void *data)
{
	TCP tcp = data;
	if (!tcp) return;

	if (tcp->cfTriggeredTimer) {
		CFRunLoopTimerInvalidate(tcp->cfTriggeredTimer);
		tcp->cfTriggeredTimer = NULL;
	}

	if (tcp->cfRunLoopSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tcp->cfRunLoopSource, kCFRunLoopCommonModes);
		CFRelease(tcp->cfRunLoopSource);
		tcp->cfRunLoopSource = NULL;
	}

	if (tcp->cfSkt) {
		CFSocketInvalidate(tcp->cfSkt);
		CFRelease(tcp->cfSkt);
		tcp->cfSkt = NULL;
		tcp->skt = -1;
	}

	if (tcp->skt >= 0) {
		close(tcp->skt);
		tcp->skt = -1;
	}
	
	if (tcp->handle) {
		free(tcp->handle);
		tcp->handle = NULL;
	}
	
	free(tcp);
}

void doClose(xsMachine *the, xsSlot *instance)
{
	TCP tcp = xsmcGetHostData(*instance);
	if (tcp && xsmcGetHostDataValidate(*instance, (void *)&xsTCPHooks)) {
		tcp->done = 1;
		tcp->triggerable = 0;
		tcp->triggered = 0;

		CFSocketDisableCallBacks(tcp->cfSkt, kCFSocketReadCallBack | kCFSocketWriteCallBack | kCFSocketConnectCallBack);

		xsmcSetHostData(*instance, NULL);
		xsForget(tcp->obj);
		xsmcSetHostDestructor(*instance, NULL);
		tcpRelease(tcp);
	}
}

void xs_tcp_close(xsMachine *the)
{
	doClose(the, &xsThis);
}

void xs_tcp_read(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	uint32_t requested;
	uint8_t *out, value;

	if (!tcp->bytesReadable)
		return;

	if (kIOFormatBuffer == tcp->format) {
		uint8_t allocate = 1;
		xsUnsignedValue byteLength;

		if (0 == xsmcArgc)
			requested = tcp->bytesReadable;
		else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
			xsResult = xsArg(0);
			xsmcGetBufferWritable(xsResult, (void **)&out, &byteLength);
			requested = (int)byteLength;
			xsmcSetInteger(xsResult, requested);
			allocate = 0;
		}
		else
			requested = xsmcToInteger(xsArg(0));

		if ((requested <= 0) || (requested > tcp->bytesReadable)) 
			xsUnknownError("invalid");

		if (allocate)
			out = xsmcSetArrayBuffer(xsResult, NULL, requested);
	}
	else {
		requested = 1;
		out = &value;
	}

	c_memmove(out, tcp->readBuf + tcp->readPosition, requested);
	tcp->readPosition += requested;
	tcp->bytesReadable -= requested;

	if (tcp->error && !tcp->bytesReadable)
		tcpTrigger(tcp, kTCPError);

	if (kIOFormatNumber == tcp->format)
		xsmcSetInteger(xsResult, value);
}

void xs_tcp_write(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	xsUnsignedValue needed;
	void *buffer;
	uint8_t value;

	if (!tcp->skt) {
		xsTrace("write to closed socket\n");
		return;
	}

	if (kIOFormatBuffer == tcp->format) {
		xsmcGetBufferReadable(xsArg(0), &buffer, &needed);
	}
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}

	if (needed > tcp->bytesWritable)
		xsUnknownError("would block");

	tcp->bytesWritable -= needed;

	CFSocketEnableCallBacks(tcp->cfSkt, kCFSocketReadCallBack | kCFSocketWriteCallBack);
	int ret = write(tcp->skt, buffer, needed);
	if (ret <= 0)
		xsUnknownError("write failed");
}

void xs_tcp_get_format(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	builtinGetFormat(the, tcp->format);
}

void xs_tcp_set_format(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("unimplemented");
	tcp->format = format;
}

void tcpHold(TCP tcp)
{
	tcp->useCount += 1;
}

void tcpRelease(TCP tcp)
{
	tcp->useCount -= 1;
	if (tcp->useCount <= 0)
		xs_tcp_destructor(tcp);
}

void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	TCP tcp = it;

	if (tcp->onReadable)
		(*markRoot)(the, tcp->onReadable);
	if (tcp->onWritable)
		(*markRoot)(the, tcp->onWritable);
	if (tcp->onError)
		(*markRoot)(the, tcp->onError);
}

void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info)
{
	TCP tcp = info;

	if (tcp->done) {
		tcpRelease(tcp);
		return;
	}

	CFArrayRef cfArray = CFHostGetAddressing(cfHost, NULL);
	if (NULL == cfArray) {	// failed to resolve
		tcpTrigger(tcp, kTCPError);
		return;
	}
	NSData *address = CFArrayGetValueAtIndex(cfArray, CFArrayGetCount(cfArray) - 1);

	const UInt8 *bytes = CFDataGetBytePtr((__bridge CFDataRef)address);
	struct sockaddr_in addr = *(struct sockaddr_in *)bytes;
	addr.sin_port = htons(tcp->port);

	CFSocketError err = CFSocketConnectToAddress(tcp->cfSkt, CFDataCreate(kCFAllocatorDefault, (const UInt8*)&addr, sizeof(addr)), (CFTimeInterval)-1);
	if (err) {
		tcpTrigger(tcp, kTCPError);
		return;
	}
}

void socketCallback(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef addr, const void *data, void *info)
{
	TCP tcp = *(TCP *)info;

	if ((-1 == tcp->skt) || tcp->done)
		return;		// closed socket

	tcpHold(tcp);

	if (cbType & kCFSocketConnectCallBack) {
		if (data) {
			// connection failed
			tcpTrigger(tcp, kTCPError);
			return;
		}
		cbType |= kCFSocketWriteCallBack;
	}

	if ((cbType & kCFSocketReadCallBack) && (tcp->bytesReadable < kBufferSize)) {
		if (tcp->readPosition) {
			if (tcp->bytesReadable)
				memmove(tcp->readBuf, tcp->readBuf + tcp->readPosition, tcp->bytesReadable);
			tcp->readPosition = 0;
		}
		int bytesRead = read(tcp->skt, tcp->readBuf + tcp->bytesReadable, kBufferSize - tcp->bytesReadable);
		if (bytesRead > 0)
			tcp->bytesReadable += bytesRead;
		else {
			tcp->error = 1;
			if (0 == tcp->bytesReadable)
				tcpTrigger(tcp, kTCPError);
			goto done;
		}
		tcpTrigger(tcp, kTCPReadable);
	}

	if (cbType & kCFSocketWriteCallBack) {
		if (kBufferSize != tcp->bytesWritable) { 
			tcp->bytesWritable = kBufferSize;
			tcpTrigger(tcp, kTCPWritable);
		}
	}

done:
	tcpRelease(tcp);
}

static void reportTrigger(CFRunLoopTimerRef cfTimer, void *info)
{
	TCP tcp = info;
	xsMachine *the = tcp->the;
	uint8_t triggered = tcp->triggered;

	tcp->triggered = 0;
	if (tcp->cfTriggeredTimer) {
		CFRunLoopTimerInvalidate(tcp->cfTriggeredTimer);
		tcp->cfTriggeredTimer = NULL;
	}

	if ((triggered & kTCPReadable) && tcp->bytesReadable) {
		xsBeginHost(the);
			xsmcSetInteger(xsResult, tcp->bytesReadable);
			xsCallFunction1(xsReference(tcp->onReadable), tcp->obj, xsResult);
		xsEndHost(the);
	}

	if ((triggered & kTCPWritable) && tcp->bytesWritable && !tcp->error) {
		xsBeginHost(the);
			xsmcSetInteger(xsResult, tcp->bytesWritable);
			xsCallFunction1(xsReference(tcp->onWritable), tcp->obj, xsResult);
		xsEndHost(the);
	}

	if (triggered & kTCPError) {
		xsBeginHost(the);
			xsCallFunction0(xsReference(tcp->onError), tcp->obj);
		xsEndHost(the);
	}

	tcpRelease(tcp);
}

void tcpTrigger(TCP tcp, uint8_t trigger)
{
	CFRunLoopTimerContext context = {0};

	if (kTCPError & trigger) {
		CFSocketDisableCallBacks(tcp->cfSkt, kCFSocketReadCallBack | kCFSocketWriteCallBack | kCFSocketConnectCallBack);
		tcp->error = 1;
	}

	trigger &= tcp->triggerable;
	if (!trigger)
		return;

	tcp->triggered |= trigger;

	if (tcp->cfTriggeredTimer)
		return;

	tcpHold(tcp);

	context.info = tcp;
	tcp->cfTriggeredTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(),
					0, 0, 0, reportTrigger, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), tcp->cfTriggeredTimer, kCFRunLoopCommonModes);
}
