/*
 * Copyright (c) 2022-2023  Moddable Tech, Inc.
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
#include "modInstrumentation.h"

#include "modTimer.h"

#include "builtinCommon.h"

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
#include <fcntl.h>

#define kBufferSize (1536)
#define kTaskInterval (20)
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
	int8_t				connected;
	int16_t				port;
	modTimer			task;
	xsMachine			*the;
	xsSlot				*onReadable;
	xsSlot				*onWritable;
	xsSlot				*onError;

	uint32_t			bytesWritable;
	uint32_t			bytesReadable;
	uint32_t			readPosition;
	uint8_t				readBuf[kBufferSize];
};
typedef struct TCPRecord TCPRecord;
typedef struct TCPRecord *TCP;

static void tcpHold(TCP tcp);
static void tcpRelease(TCP tcp);
static void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void tcpTask(modTimer timer, void *refcon, int refconSize);
static void doClose(xsMachine *the, xsSlot *instance);
static void tcpTrigger(TCP tcp, uint8_t trigger);
static void reportTrigger(TCP tcp);

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
	struct sockaddr_in addr = {0};

	xsmcVars(1);

	tcp = c_calloc(1, sizeof(TCPRecord));
	if (!tcp)
		xsRangeError("no memory");
	tcp->skt = -1;

	modInstrumentationAdjust(NetworkSockets, +1);

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
			modTimerRemove(from->task);
			from->task = NULL;
			tcp->task = modTimerAdd(kTaskInterval, kTaskInterval, tcpTask, &tcp, sizeof(tcp));
			tcp->skt = from->skt;
			tcp->port = from->port;
			tcp->bytesWritable = from->bytesWritable;
			tcp->bytesReadable = from->bytesReadable;
			tcp->readPosition = from->readPosition;
			memmove(tcp->readBuf, from->readBuf, sizeof(from->readBuf)); 
			tcp->error = from->error;

			from->skt = -1;
			from->triggered = 0;
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
			if (!inet_aton(addrStr, &addr.sin_addr))
				xsUnknownError("invalid address");;

			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = xsmcToInteger(xsVar(0));
			if ((port < 0) || (port > 65535))
				xsRangeError("invalid port");

			tcp->skt = socket(AF_INET, SOCK_STREAM, 0);
			if (tcp->skt < 0)
				xsUnknownError("no socket");

#if defined(SO_NOSIGPIPE)
			int set = 1;
			setsockopt(tcp->skt, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

			fcntl(tcp->skt, F_SETFL, O_NONBLOCK | fcntl(tcp->skt, F_GETFL, 0));

			tcp->task = modTimerAdd(kTaskInterval, kTaskInterval, tcpTask, &tcp, sizeof(tcp));

			xsmcGet(xsVar(0), xsArg(0), xsID_address);

			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			if (connect(tcp->skt, (struct sockaddr*)&addr, sizeof(addr)) >= 0)
				tcpTrigger(tcp, kTCPWritable);
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

	if (tcp->task) {
		modTimerRemove(tcp->task);
		tcp->task = NULL;
	}

	if (tcp->skt >= 0) {
		close(tcp->skt);
		tcp->skt = -1;
	}

	free(tcp);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void doClose(xsMachine *the, xsSlot *instance)
{
	TCP tcp = xsmcGetHostData(*instance);
	if (tcp && xsmcGetHostDataValidate(*instance, (void *)&xsTCPHooks)) {
		tcp->done = 1;
		tcp->triggerable = 0;
		tcp->triggered = 0;

		if (tcp->task) {
			modTimerRemove(tcp->task);
			tcp->task = NULL;
		}

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

	if (kIOFormatBuffer == tcp->format)
		xsmcGetBufferReadable(xsArg(0), &buffer, &needed);
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}

	if (needed > tcp->bytesWritable)
		xsUnknownError("would block");

	tcp->bytesWritable -= needed;

	int ret = write(tcp->skt, buffer, needed);
	if (ret < 0) {
		xsTrace("write failed");
		tcpTrigger(tcp, kTCPError);
		return;
	}

	modInstrumentationAdjust(NetworkBytesWritten, needed);
}

void xs_tcp_get_remoteAddress(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	char *out;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	getpeername(tcp->skt, (struct sockaddr *)&addr, &addrlen);		//@@ this seems wrong

	xsResult = xsStringBuffer(NULL, 4 * 5);
	out = xsmcToString(xsResult);
	inet_ntop(AF_INET, &addr, out, 4 * 5);
}

void xs_tcp_get_remotePort(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	getpeername(tcp->skt, (struct sockaddr *)&addr, &addrlen);
	xsmcSetInteger(xsResult, ntohs(addr.sin_port));
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

void tcpTask(modTimer timer, void *refcon, int refconSize)
{
	TCP tcp = *(TCP *)refcon;
	fd_set rfds, wfds;
	struct timeval tv;

	if ((-1 == tcp->skt) || tcp->done)
		return;		// closed socket

	tcpHold(tcp);

	FD_ZERO(&wfds);
	FD_ZERO(&rfds);

	if (!tcp->connected || (kBufferSize != tcp->bytesWritable))
		FD_SET(tcp->skt, &wfds);
	if (tcp->bytesReadable < kBufferSize)
		FD_SET(tcp->skt, &rfds);

	tv.tv_sec = tv.tv_usec = 0;
	int result = select(tcp->skt + 1, &rfds, &wfds, NULL, &tv);
	if (result > 0) {
		if (FD_ISSET(tcp->skt, &wfds)) {
			tcp->connected = true;
			if (kBufferSize != tcp->bytesWritable) { 
				tcp->bytesWritable = kBufferSize;
				tcpTrigger(tcp, kTCPWritable);
			}
		}
		if (FD_ISSET(tcp->skt, &rfds) && (tcp->bytesReadable < kBufferSize)) {
			if (tcp->readPosition) {
				if (tcp->bytesReadable)
					memmove(tcp->readBuf, tcp->readBuf + tcp->readPosition, tcp->bytesReadable);
				tcp->readPosition = 0;
			}
			int bytesRead = read(tcp->skt, tcp->readBuf + tcp->bytesReadable, kBufferSize - tcp->bytesReadable);
			if (bytesRead > 0) {
				tcp->bytesReadable += bytesRead;
				tcpTrigger(tcp, kTCPReadable);
				modInstrumentationAdjust(NetworkBytesRead, bytesRead);
			}
			else if (bytesRead < 0) {
				tcp->error = 1;
				if (0 == tcp->bytesReadable)
					tcpTrigger(tcp, kTCPError);
			}
		}
	}

	if (tcp->triggered)
		reportTrigger(tcp);

	tcpRelease(tcp);
}

void reportTrigger(TCP tcp)
{
	xsMachine *the = tcp->the;
	uint8_t triggered = tcp->triggered;

	tcp->triggered = 0;

	if ((triggered & kTCPWritable) && tcp->bytesWritable && !tcp->error) {
		xsBeginHost(the);
			xsmcSetInteger(xsResult, tcp->bytesWritable);
			xsCallFunction1(xsReference(tcp->onWritable), tcp->obj, xsResult);
		xsEndHost(the);
	}

	if ((triggered & kTCPReadable) && tcp->bytesReadable) {
		xsBeginHost(the);
			xsmcSetInteger(xsResult, tcp->bytesReadable);
			xsCallFunction1(xsReference(tcp->onReadable), tcp->obj, xsResult);
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
	if (kTCPError & trigger)
		tcp->error = 1;

	trigger &= tcp->triggerable;
	if (!trigger)
		return;

	if (!tcp->triggered)
		tcpHold(tcp);
	
	tcp->triggered |= trigger;
}

/*
	Listener
 */

struct ListenerPendingRecord {
	struct ListenerPendingRecord	*next;
    int								skt;
};
typedef struct ListenerPendingRecord ListenerPendingRecord;
typedef struct ListenerPendingRecord *ListenerPending;

struct ListenerRecord {

	xsSlot				obj;
	ListenerPending		pending;

	xsMachine			*the;
	xsSlot				*onReadable;
//	xsSlot				*onError;

	int					skt;
	modTimer			task;
};
typedef struct ListenerRecord ListenerRecord;
typedef struct ListenerRecord *Listener;

static void listenerTask(modTimer timer, void *refcon, int refconSize);

static void xs_listener_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static const xsHostHooks xsListenerHooks = {
	xs_listener_destructor_,
	xs_listener_mark,
	NULL
};

void xs_listener_constructor(xsMachine *the)
{
	Listener listener;
	uint16_t port = 0;
	xsSlot *onReadable;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}

	onReadable = builtinGetCallback(the, xsID_onReadable);
	// hasOnError

	if (kIOFormatSocketTCP != builtinInitializeFormat(the, kIOFormatSocketTCP))
		xsRangeError("unimplemented");

	xsTry {
		struct sockaddr_in address = { 0 };

		listener = c_calloc(sizeof(ListenerRecord), 1);
		listener->the = the;
		listener->obj = xsThis;
		xsmcSetHostData(xsThis, listener);
		xsRemember(listener->obj);

		listener->task = modTimerAdd(kTaskInterval, kTaskInterval, listenerTask, &listener, sizeof(listener));

		listener->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (-1 == listener->skt)
			xsUnknownError("create socket failed");

		fcntl(listener->skt, F_SETFL, O_NONBLOCK | fcntl(listener->skt, F_GETFL, 0));

		int yes = 1;
		setsockopt(listener->skt, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		if (-1 == bind(listener->skt, (struct sockaddr *)&address, sizeof(address)))
			xsUnknownError("bind socket failed");

		if (-1 == listen(listener->skt, SOMAXCONN))
			xsUnknownError("listen failed");

		builtinInitializeTarget(the);

		listener->onReadable = onReadable;
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsListenerHooks);
	}
	xsCatch {
		xsmcSetHostData(xsThis, NULL);
		xs_listener_destructor_(listener);
		xsThrow(xsException);
	}
}

void xs_listener_destructor_(void *data)
{
	Listener listener = data;
	if (!data) return;

	if (listener->task) {
		modTimerRemove(listener->task);
		listener->task = NULL;
	}

	if (listener->skt) {
		close(listener->skt);
		listener->skt = 0;
	}

	while (listener->pending) {
		ListenerPending pending = listener->pending;
		listener->pending = pending->next;
		close(pending->skt);
		c_free(pending);
	}

	c_free(listener);
}

void xs_listener_close_(xsMachine *the)
{
	Listener listener = xsmcGetHostData(xsThis);
	if (listener && xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks)) {
		xsForget(listener->obj);
		xs_listener_destructor_(listener);
		xsmcSetHostData(xsThis, NULL);
	}
}

void xs_listener_read(xsMachine *the)
{
	Listener listener = xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks);
	ListenerPending pending;
	TCP tcp;

	pending = listener->pending;
	if (!pending)
		return;

	listener->pending = pending->next;

	xsResult = xsArg(0);
	tcp = xsmcGetHostDataValidate(xsResult, (void *)&xsTCPHooks);

	tcp->skt = pending->skt;
	c_free(pending);
}

void xs_listener_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Listener listener = it;

	if (listener->onReadable)
		(*markRoot)(the, listener->onReadable);
}

static void listenerTask(modTimer timer, void *refcon, int refconSize)
{
	Listener listener = *(Listener *)refcon;
	int sockets = 0;

	while (true) {
		int skt = accept(listener->skt, NULL, NULL);
		if (skt <= 0)
			break;

		ListenerPending lp = c_calloc(sizeof(ListenerPendingRecord), 1);
		lp->skt = skt;
		
		if (NULL == listener->pending)
			listener->pending = lp;
		else {
			ListenerPending walker = listener->pending;
			while (walker->next)
				walker = walker->next;
			walker->next = lp;
		}

		sockets++;
	}

	if (!sockets || !listener->onReadable)
		return;

	ListenerPending walker;
	for (walker = listener->pending, sockets = 0; walker; walker = walker->next)
		sockets++;

	xsMachine *the = listener->the;
	xsBeginHost(the);
		xsmcSetInteger(xsResult, sockets);
		xsCallFunction1(xsReference(listener->onReadable), listener->obj, xsResult);
	xsEndHost(the);
}
