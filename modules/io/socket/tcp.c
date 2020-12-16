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
	TCP socket - uing lwip low level callback API

	To do:
		ASCII
		deliver details to onError (disconnect, etc)
		allow collection (xsForget) once error /  disconnect
		while connecting cannot safely transfer native socket

		unsafe on ESP32 - assumes single thread
*/

#include "lwip/tcp.h"

#include "xsmc.h"			// xs bindings for microcontroller
#ifdef __ets__
	#include "xsHost.h"		// esp platform support
#else
	#error - unsupported platform
#endif
#include "mc.xs.h"			// for xsID_* values
#include "builtinCommon.h"

struct TCPBufferRecord {
	struct TCPBufferRecord *next;
	struct pbuf		*pb;
	struct pbuf		*fragment;		// next, payload, len
	uint16_t		fragmentOffset;	// offset into fragment buffer
	uint16_t		bytes;			// unconsumed bytes in pb
};
typedef struct TCPBufferRecord TCPBufferRecord;
typedef struct TCPBufferRecord *TCPBuffer;

enum {
	kTCPWritable = 1 << 0,
	kTCPReadable = 1 << 1,
	kTCPError = 1 << 2,
	kTCPOutput = 1 << 3,
};

struct TCPRecord {
	struct tcp_pcb	*skt;
	xsSlot			obj;
	TCPBuffer		buffers;
	uint8_t			triggerable;
	uint8_t			triggered;
	int8_t			useCount;
	uint8_t			format;
	xsMachine		*the;
	xsSlot			onReadable;
	xsSlot			onWritable;
	xsSlot			onError;
};
typedef struct TCPRecord TCPRecord;
typedef struct TCPRecord *TCP;

static void tcpHold(TCP tcp);
static void tcpRelease(TCP tcp);

static err_t tcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcpError(void *arg, err_t err);
static err_t tcpReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcpSent(void *arg, struct tcp_pcb *pcb, u16_t len);
static void tcpTrigger(TCP tcp, uint8_t trigger);

static void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_tcp_constructor(xsMachine *the)
{
	TCP tcp;
	uint8_t create = xsmcArgc > 0;
	uint8_t connect = 0, triggerable = 0, triggered, nodelay = 0, format = kIOFormatBuffer;
	int port;
	struct tcp_pcb *skt;
	TCPBuffer buffers = NULL;
	ip_addr_t address;

	xsmcVars(1);

	if (create) {
		format = builtinInitializeFormat(the, format);
		if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
			xsRangeError("unimplemented");

		if (builtinHasCallback(the, xsID_onReadable))
			triggerable |= kTCPReadable;
		if (builtinHasCallback(the, xsID_onWritable))
			triggerable |= kTCPWritable;
		if (builtinHasCallback(the, xsID_onError))
			triggerable |= kTCPError;

		if (xsmcHas(xsArg(0), xsID_nodelay)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_nodelay);
			nodelay = xsmcTest(xsVar(0)) ? 2 : 1;
		}

		if (xsmcHas(xsArg(0), xsID_from)) {
			TCP from;

			xsmcGet(xsVar(0), xsArg(0), xsID_from);
			from = xsmcGetHostData(xsVar(0));
			if (!from || !from->skt)
				xsUnknownError("invalid from");

			skt = from->skt;
			buffers = from->buffers;
			triggered = from->triggered;
			if ((triggerable & kTCPWritable) && tcp_sndbuf(skt))
				triggered |= kTCPWritable;
			if ((triggerable & kTCPReadable) && buffers)
				triggered |= kTCPReadable;
			from->skt = NULL;
			from->buffers = NULL;
			from->triggered = 0;		// prevent outstanding delivery from doing anything
		}
		else {
			char addrStr[32];

			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			if (!ipaddr_aton(xsmcToStringBuffer(xsVar(0), addrStr, sizeof(addrStr)), &address))
				xsRangeError("invalid IP address");

			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = xsmcToInteger(xsVar(0));
			if ((port < 0) || (port > 65535))
				xsRangeError("invalid port");

			skt = tcp_new();
			if (!skt)
				xsRangeError("no socket");

			skt->so_options |= SOF_REUSEADDR;
			if (tcp_bind(skt, IP_ADDR_ANY, 0)) {
				tcp_close(skt);
				xsUnknownError("bind failed");
			}

			connect = 1;
		}

		tcp = c_calloc(1, triggerable ? sizeof(TCPRecord) : offsetof(TCPRecord, onReadable));
	}
	else
		tcp = c_calloc(1, offsetof(TCPRecord, onReadable));

	if (!tcp) {
		tcp_close(skt);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, tcp);
	tcp->the = the;
	tcp->obj = xsThis;
	xsRemember(tcp->obj);
	tcp->format = format;
	builtinInitializeTarget(the);
	tcpHold(tcp);

	if (nodelay) {
		if (2 == nodelay)
			tcp_nagle_disable(skt);
		else
			tcp_nagle_enable(skt);
	}

	if (!create)
		return;

	tcp->skt = skt;
	tcp->buffers = buffers;

	tcp_arg(skt, tcp);
	tcp_err(skt, tcpError);
	tcp_recv(skt, tcpReceive);

	if (triggerable) {
		tcp->triggerable = triggerable;

		if (triggerable & kTCPReadable) {
			builtinGetCallback(the, xsID_onReadable, &tcp->onReadable);
			xsRemember(tcp->onReadable);
		}

		if (triggerable & kTCPWritable) {
			builtinGetCallback(the, xsID_onWritable, &tcp->onWritable);
			xsRemember(tcp->onWritable);

			tcp_sent(skt, tcpSent);
		}

		if (triggerable & kTCPError) {
			builtinGetCallback(the, xsID_onError, &tcp->onError);
			xsRemember(tcp->onError);
		}
	}

	if (connect) {
		if (tcp_connect(skt, &address, port, tcpConnect))
			xsUnknownError("connect error");
	}
	else if (triggered)
		tcpTrigger(tcp, triggered);
}

void xs_tcp_destructor(void *data)
{
	TCP tcp = data;
	if (!tcp) return;

	while (tcp->buffers) {
		TCPBuffer buffer = tcp->buffers;
		tcp->buffers = buffer->next;
		pbuf_free(buffer->pb);
		c_free(buffer);
	}

	if (tcp->skt) {
		tcp_recv(tcp->skt, NULL);
		tcp_sent(tcp->skt, NULL);
		tcp_err(tcp->skt, NULL);

		tcp_close(tcp->skt);
#if !ESP32
		tcp_abort(tcp->skt);
#endif
	}

	c_free(data);
}

void xs_tcp_close(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
	if (!tcp) return;

	xsmcSetHostData(xsThis, NULL);
	xsForget(tcp->obj);
	if (tcp->triggerable) {
		if (tcp->triggerable & kTCPReadable)
			xsForget(tcp->onReadable);
		if (tcp->triggerable & kTCPWritable)
			xsForget(tcp->onWritable);
		if (tcp->triggerable & kTCPError)
			xsForget(tcp->onError);
		tcp->triggerable = 0;
	}

	tcpRelease(tcp);
}

void xs_tcp_read(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
	TCPBuffer buffer;
	int requested;
	uint8_t *out, value;

	if (!tcp || !tcp->skt)
		xsUnknownError("bad state");

	if (!tcp->buffers)
		return;

	if (kIOFormatBuffer == tcp->format) {
		int available = 0;

		requested = (xsmcArgc > 0) ? xsmcToInteger(xsArg(0)) : 0x7FFFFFFF;
		if (!requested)
			return;

		for (buffer = tcp->buffers; NULL != buffer; buffer = buffer->next)
			available += buffer->bytes;

		if (available < requested)
			requested = available;

		xsmcSetArrayBuffer(xsResult, NULL, requested);
		out = xsmcToArrayBuffer(xsResult);
	}
	else {
		requested = 1;
		out = &value;
	}

	while (requested) {
		int use = requested;
		TCPBuffer buffer = tcp->buffers;
		struct pbuf *fragment = buffer->fragment;

		if (use > (fragment->len - buffer->fragmentOffset))
			use = fragment->len - buffer->fragmentOffset;

		c_memmove(out, buffer->fragmentOffset + (uint8_t *)fragment->payload, use);

		out += use;
		requested -= use;
		buffer->bytes -= use;
		buffer->fragmentOffset += use;

		if (buffer->fragmentOffset == fragment->len) {
			buffer->fragment = fragment->next;
			buffer->fragmentOffset = 0;
			if (NULL == buffer->fragment) {
				tcp->buffers = buffer->next;
				tcp_recved(tcp->skt, buffer->pb->tot_len);
				pbuf_free(buffer->pb);
				c_free(buffer);
				if (NULL == tcp->buffers)
					break;
			}
		}
	}

	if (kIOFormatNumber == tcp->format)
		xsmcSetInteger(xsResult, value);
}

void xs_tcp_write(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
	int needed;
	void *buffer;
	uint8_t value;

	if (!tcp || !tcp->skt)
		xsUnknownError("bad state");

	if (kIOFormatBuffer == tcp->format) {
		if (xsmcIsInstanceOf(xsArg(0), xsTypedArrayPrototype))
			xsmcGet(xsArg(0), xsArg(0), xsID_buffer);

		needed = xsmcGetArrayBufferLength(xsArg(0));
		buffer = xsmcToArrayBuffer(xsArg(0));
	}
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}

	if (needed > tcp_sndbuf(tcp->skt))
		xsUnknownError("would block");

	if (ERR_OK != tcp_write(tcp->skt, buffer, needed, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE))
		xsUnknownError("write failed");

	tcpTrigger(tcp, kTCPOutput);
}

void xs_tcp_get_format(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
	builtinGetFormat(the, tcp->format);
}

void xs_tcp_set_format(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
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
	if (0 == tcp->useCount)
		xs_tcp_destructor(tcp);
}

void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	TCP tcp = refcon;
	uint8_t triggered = tcp->triggered;		//@@ protect on ESP32
	tcp->triggered = 0;

	if (triggered & kTCPOutput)
		tcp_output(tcp->skt);

	if ((triggered & kTCPReadable) && tcp->buffers) {
		TCPBuffer walker;
		int bytes = 0;

		for (walker = tcp->buffers; NULL != walker; walker = walker->next)
			bytes += walker->bytes;

		xsBeginHost(the);
			xsmcSetInteger(xsResult, bytes);
			xsCallFunction1(tcp->onReadable, tcp->obj, xsResult);
		xsEndHost(the);
	}

	if (!(triggered & kTCPError)) {
		if (triggered & kTCPWritable) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, tcp_sndbuf(tcp->skt));
				xsCallFunction1(tcp->onWritable, tcp->obj, xsResult);
			xsEndHost(the);
		}
	}
	else if (tcp->triggerable & kTCPError) {
		xsBeginHost(the);
			xsCallFunction0(tcp->onError, tcp->obj);
		xsEndHost(the);
	}

	tcpRelease(tcp);
}

err_t tcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	tcpTrigger((TCP)arg, err ? kTCPError : kTCPWritable);
	return ERR_OK;
}

void tcpError(void *arg, err_t err)
{
	TCP tcp = arg;

	tcp->skt = NULL;		// "pcb is already freed when this callback is called"
	if (kTCPError & tcp->triggerable)
		tcpTrigger(tcp, kTCPError);
}

err_t tcpReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, err_t err)
{
	TCP tcp = arg;
	TCPBuffer buffer;

	if ((NULL == pb) || (ERR_OK != err)) {		//@@ when is err set here?
#if ESP32
		tcp->skt = NULL;			// no close on socket if disconnected.
#endif
		tcpTrigger(tcp, kTCPError);
		return ERR_OK;
	}

#ifdef mxDebug
	if (pb && fxInNetworkDebugLoop(tcp->the))
		return ERR_MEM;
#endif

	buffer = c_malloc(sizeof(TCPBufferRecord));
	if (!buffer)
		return ERR_MEM;		// retry later

	buffer->next = NULL;
	buffer->pb = pb;
	buffer->bytes = pb->tot_len;
	buffer->fragment = pb;
	buffer->fragmentOffset = 0;

	if (tcp->buffers) {
		TCPBuffer walker;

		for (walker = tcp->buffers; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	else
		tcp->buffers = buffer;

	if (tcp->triggerable & kTCPReadable)
		tcpTrigger(tcp, kTCPReadable);

	return ERR_OK;
}

err_t tcpSent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	tcpTrigger((TCP)arg, kTCPWritable);
	return ERR_OK;
}

void tcpTrigger(TCP tcp, uint8_t trigger)
{
	uint8_t triggered = tcp->triggered;		//@@ protect on ESP32

	tcp->triggered |= trigger;
	if (!triggered) {
		tcpHold(tcp);
		modMessagePostToMachine(tcp->the, NULL, 0, tcpDeliver, tcp);
	}
}

/*
	Listener
*/

struct ListenerPendingRecord {
	struct ListenerPendingRecord *next;
	struct tcp_pcb	*skt;
};
typedef struct ListenerPendingRecord ListenerPendingRecord;
typedef struct ListenerPendingRecord *ListenerPending;

struct ListenerRecord {
	struct tcp_pcb	*skt;
	xsSlot			obj;
	ListenerPending	pending;
	uint8_t			hasOnReadable;
//	uint8_t			hasOnError;
	uint8_t			triggered;
	xsMachine		*the;
	xsSlot			onReadable;
//	xsSlot			onError;
};
typedef struct ListenerRecord ListenerRecord;
typedef struct ListenerRecord *Listener;

static err_t listenerAccept(void *arg, struct tcp_pcb *skt, err_t err);
static void listenerDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void listenerTrigger(Listener listener, uint8_t trigger);

enum {
	kListenerReadable = 1 << 0,
//	kListenerError = 1 << 3,
};

void xs_listener_constructor(xsMachine *the)
{
	Listener listener;
	struct tcp_pcb *skt;
	ip_addr_t address = *(IP_ADDR_ANY);
	uint16_t port = 0;
	uint8_t hasOnReadable = 0;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16)xsmcToInteger(xsVar(0));
	}
	//@@ address

	hasOnReadable = builtinHasCallback(the, xsID_onReadable);
	// hasOnError

	if (kIOFormatSocketTCP != builtinInitializeFormat(the, kIOFormatSocketTCP))
		xsRangeError("unimplemented");

	skt = tcp_new();
	if (!skt)
		xsUnknownError("no socket");

	skt->so_options |= SOF_REUSEADDR;
	if (tcp_bind(skt, &address, port)) {
		tcp_close(skt);
		xsUnknownError("socket bind");
	}

	skt = tcp_listen(skt);
	if (!skt) {
		tcp_close(skt);
		xsRangeError("no memory");
	}

	listener = c_calloc(1, hasOnReadable ? sizeof(ListenerRecord) : offsetof(ListenerRecord, onReadable));
	if (!listener) {
		tcp_close(skt);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, listener);
	listener->obj = xsThis;
	xsRemember(listener->obj);
	listener->skt = skt;
	listener->the = the;

	builtinInitializeTarget(the);

	tcp_arg(skt, listener);
	tcp_accept(skt, listenerAccept);

	if (hasOnReadable) {
		builtinGetCallback(the, xsID_onReadable, &listener->onReadable);
		listener->hasOnReadable = 1;
		xsRemember(listener->onReadable);
	}
}

void xs_listener_destructor_(void *data)
{
	Listener listener = data;
	if (!data) return;

	while (listener->pending) {
		ListenerPending pending = listener->pending;
		listener->pending = pending->next;
		tcp_close(pending->skt);
		c_free(pending);
	}

	tcp_accept(listener->skt, NULL);
	tcp_close(listener->skt);

	c_free(listener);
}

void xs_listener_close_(xsMachine *the)
{
	Listener listener = xsmcGetHostData(xsThis);
	if (!listener) return;

	xsForget(listener->obj);
	if (listener->hasOnReadable)
		xsForget(listener->onReadable);
	xs_listener_destructor_(listener);
	xsmcSetHostData(xsThis, NULL);
}

void xs_listener_read(xsMachine *the)
{
	Listener listener = xsmcGetHostData(xsThis);
	ListenerPending pending = listener->pending;
	TCP tcp;

	if (NULL == pending) {
		xsDebugger();
		xsCall0(xsArg(0), xsID_close);
		return;
	}

	listener->pending = pending->next;

	xsResult = xsArg(0);
	tcp = xsmcGetHostData(xsArg(0));

	tcp->skt = pending->skt;
	c_free(pending);

	tcp_accepted(tcp->skt);

	tcp_arg(tcp->skt, tcp);
	tcp_err(tcp->skt, tcpError);
	tcp_recv(tcp->skt, tcpReceive);
}

void listenerDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Listener listener = refcon;
	uint8_t triggered = listener->triggered;		//@@ protect on ESP32
	listener->triggered = 0;

	if ((triggered & kListenerReadable) && listener->pending) {
		ListenerPending walker;
		int sockets = 0;

		for (walker = listener->pending; NULL != walker; walker = walker->next)
			sockets++;

		xsBeginHost(the);
			xsmcSetInteger(xsResult, sockets);
			xsCallFunction1(listener->onReadable, listener->obj, xsResult);
		xsEndHost(the);
	}

//	if (triggered & kListenerError) {
//	}
}

err_t listenerReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	return ERR_MEM;		// wait until connection accepted
}

err_t listenerAccept(void *arg, struct tcp_pcb *skt, err_t err)
{
	Listener listener = arg;
	ListenerPending pending, walker;

#ifdef mxDebug
	if (fxInNetworkDebugLoop(listener->the))
		return ERR_ABRT;		// lwip will close
#endif

	pending = c_malloc(sizeof(ListenerPendingRecord));
	pending->next = NULL;
	pending->skt = skt;

	tcp_recv(skt, listenerReceive);

	walker = listener->pending;
	if (NULL == walker)
		listener->pending = pending;
	else {
		while (walker->next)
			walker = walker->next;
		walker->next = pending;
	}

	if (listener->hasOnReadable)
		listenerTrigger(listener, kListenerReadable);

	return ERR_OK;
}

void listenerTrigger(Listener listener, uint8_t trigger)
{
	uint8_t triggered = listener->triggered;		//@@ protect on ESP32

	listener->triggered |= trigger;
	if (!triggered)
		modMessagePostToMachine(listener->the, NULL, 0, listenerDeliver, listener);
}
