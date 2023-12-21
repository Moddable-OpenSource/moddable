/*
 * Copyright (c) 2019-2023  Moddable Tech, Inc.
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
		deliver details to onError (disconnect, etc)
		allow collection (xsForget) once error /  disconnect
		while connecting cannot safely transfer native socket
		
		on ESP8266, data in flash needs to be spooled through RAM before sending to lwip
*/

#include "lwip/tcp.h"

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"		// esp platform support
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values
#include "builtinCommon.h"
#include "modLwipSafe.h"

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
	uint8_t			error;
	uint8_t			format;
	xsMachine		*the;
	xsSlot			*onReadable;
	xsSlot			*onWritable;
	xsSlot			*onError;
};
typedef struct TCPRecord TCPRecord;
typedef struct TCPRecord *TCP;

static void removeTCPCallbacks(TCP tcp);
static void tcpHold(TCP tcp);
static void tcpRelease(TCP tcp);

static err_t tcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcpError(void *arg, err_t err);
static err_t tcpReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcpSent(void *arg, struct tcp_pcb *pcb, u16_t len);
static void tcpTrigger(TCP tcp, uint8_t trigger);

static void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void doClose(xsMachine *the, xsSlot *instance);

static const xsHostHooks ICACHE_RODATA_ATTR xsTCPHooks = {
	xs_tcp_destructor,
	xs_tcp_mark,
	NULL
};

void xs_tcp_constructor(xsMachine *the)
{
	TCP tcp;
	uint8_t create = xsmcArgc > 0;
	uint8_t connect = 0, triggerable = 0, triggered = 0, nodelay = 0, format = kIOFormatBuffer;
	xsSlot *onReadable, *onWritable, *onError;
	int port;
	struct tcp_pcb *skt;
	TCPBuffer buffers = NULL;
	ip_addr_t address;

	xsmcVars(1);

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

			skt = from->skt;
			buffers = from->buffers;
			triggered = from->triggered;
			if ((triggerable & kTCPWritable) && tcp_sndbuf(skt))
				triggered |= kTCPWritable;
			if ((triggerable & kTCPReadable) && buffers)
				triggered |= kTCPReadable;
			from->skt = NULL;
			from->buffers = NULL;
			from->triggered = 0;
			doClose(the, &xsVar(0));
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

			skt = tcp_new_safe();
			if (!skt)
				xsRangeError("no socket");

			skt->so_options |= SOF_REUSEADDR;
			if (tcp_bind_safe(skt, IP_ADDR_ANY, 0)) {
				tcp_close_safe(skt);
				xsUnknownError("bind failed");
			}

			connect = 1;
		}

		tcp = c_calloc(1, triggerable ? sizeof(TCPRecord) : offsetof(TCPRecord, onReadable));
	}
	else
		tcp = c_calloc(1, offsetof(TCPRecord, onReadable));

	if (!tcp) {
		tcp_close_safe(skt);
		xsRangeError("no memory");
	}

	modInstrumentationAdjust(NetworkSockets, +1);

	xsmcSetHostData(xsThis, tcp);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsTCPHooks);
	tcp->the = the;
	tcp->obj = xsThis;
	xsRemember(tcp->obj);
	tcp->format = format;
	tcpHold(tcp);

	if (!create)
		return;

	if (nodelay) {
		if (2 == nodelay)
			tcp_nagle_disable(skt);
		else
			tcp_nagle_enable(skt);
	}

	builtinInitializeTarget(the);

	tcp->skt = skt;
	tcp->buffers = buffers;

	tcp_arg(skt, tcp);
	tcp_err(skt, tcpError);
	tcp_recv(skt, tcpReceive);

	tcp->triggerable = triggerable;
	tcp->onReadable = onReadable;
	tcp->onWritable = onWritable;
	tcp->onError = onError;

	if (onWritable)
		tcp_sent(skt, tcpSent);

	if (connect) {
		if (tcp_connect_safe(skt, &address, port, tcpConnect))
			xsUnknownError("connect error");
	}
	else if (triggered)
		tcpTrigger(tcp, triggered);
}

void xs_tcp_destructor(void *data)
{
	TCP tcp = data;
	if (!tcp) return;

	if (tcp->skt) {
		removeTCPCallbacks(tcp);

		tcp_close_safe(tcp->skt);
	}

	while (tcp->buffers) {
		TCPBuffer buffer = tcp->buffers;
		tcp->buffers = buffer->next;
		pbuf_free_safe(buffer->pb);
		c_free(buffer);
	}

	c_free(data);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void removeTCPCallbacks(TCP tcp)
{
	if (tcp->skt) {
		tcp_recv(tcp->skt, NULL);
		tcp_sent(tcp->skt, NULL);
		tcp_err(tcp->skt, NULL);
	}

	tcp->triggerable &= ~kTCPWritable;
}

void doClose(xsMachine *the, xsSlot *instance)
{
	TCP tcp = xsmcGetHostData(*instance);
	if (tcp && xsmcGetHostDataValidate(*instance, (void *)&xsTCPHooks)) {
		removeTCPCallbacks(tcp);
		tcp->triggerable = 0;

		xsmcSetHostData(*instance, NULL);
		xsForget(tcp->obj);
		xsmcSetHostDestructor(*instance, NULL);
#if TCP_ATOMICS
		modAtomicsExchange_n(&tcp->triggered, 0);
#else
		builtinCriticalSectionBegin();
		tcp->triggered = 0;
		builtinCriticalSectionEnd();
#endif
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
	TCPBuffer buffer;
	int requested;
	uint8_t *out, value;

	if (!tcp->buffers)
		return;

	if (kIOFormatBuffer == tcp->format) {
		int available = 0;
		uint8_t allocate = 1;
		xsUnsignedValue byteLength;

		for (buffer = tcp->buffers; NULL != buffer; buffer = buffer->next)
			available += buffer->bytes;

		if (0 == xsmcArgc)
			requested = available;
		else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
			xsResult = xsArg(0);
			xsmcGetBufferWritable(xsResult, (void **)&out, &byteLength);
			requested = (int)byteLength;
			allocate = 0;
		}
		else
			requested = xsmcToInteger(xsArg(0));

		if ((requested <= 0) || (requested > available)) 
			xsUnknownError("invalid");

		if (allocate)
			out = xsmcSetArrayBuffer(xsResult, NULL, requested);
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
				tcp_recved_safe(tcp->skt, buffer->pb->tot_len);
				pbuf_free_safe(buffer->pb);
				c_free(buffer);
				if (NULL == tcp->buffers) {
					if (tcp->error)
						tcpTrigger(tcp, kTCPError);
					break;
				}
			}
		}
	}

	if (kIOFormatNumber == tcp->format)
		xsmcSetInteger(xsResult, value);
}

void xs_tcp_write(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	xsUnsignedValue needed;
	void *buffer;
	uint8_t value;

	if (tcp->error || !tcp->skt)
		return;

	if (kIOFormatBuffer == tcp->format)
		xsmcGetBufferReadable(xsArg(0), &buffer, &needed);
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}

	if (needed > tcp_sndbuf(tcp->skt))
		xsUnknownError("would block");

	if (ERR_OK != tcp_write_safe(tcp->skt, buffer, needed, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE)) {
		xsTrace("tcp write failed\n");
		tcpTrigger(tcp, kTCPError);
		return;
	}

	tcpTrigger(tcp, kTCPOutput);

	modInstrumentationAdjust(NetworkBytesWritten, needed);
}

void xs_tcp_get_remoteAddress(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);

	xsResult = xsStringBuffer(NULL, 40);
	ipaddr_ntoa_r(&tcp->skt->remote_ip, xsmcToString(xsResult), 40);
}

void xs_tcp_get_remotePort(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);

	xsmcSetInteger(xsResult, tcp->skt->remote_port);
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
#if TCP_ATOMICS
	modAtomicsAddFetch(&tcp->useCount, 1);
#else
	builtinCriticalSectionBegin();
	tcp->useCount += 1;
	builtinCriticalSectionEnd();
#endif
}

void tcpRelease(TCP tcp)
{
#if TCP_ATOMICS
	if (0 == modAtomicsSubFetch(&tcp->useCount, 1))
		xs_tcp_destructor(tcp);
#else
	uint8_t unused;

	builtinCriticalSectionBegin();
	tcp->useCount -= 1;
	unused = 0 == tcp->useCount;
	builtinCriticalSectionEnd();
	if (unused)
		xs_tcp_destructor(tcp);
#endif

}

void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	TCP tcp = refcon;
#if TCP_ATOMICS
	uint8_t triggered = modAtomicsExchange_n(&tcp->triggered, 0);
#else
	builtinCriticalSectionBegin();
	uint8_t triggered = tcp->triggered;
	tcp->triggered = 0;
	builtinCriticalSectionEnd();
#endif
	if (triggered & kTCPError) {
		triggered &= ~(kTCPOutput | kTCPWritable);
		tcp->error = true;
	}

	if ((triggered & kTCPOutput) && tcp->skt)
		tcp_output_safe(tcp->skt);

	if ((triggered & tcp->triggerable & kTCPReadable) && tcp->buffers) {
		TCPBuffer walker;
		int bytes = 0;

		for (walker = tcp->buffers; NULL != walker; walker = walker->next)
			bytes += walker->bytes;

		xsBeginHost(the);
			xsmcSetInteger(xsResult, bytes);
			xsCallFunction1(xsReference(tcp->onReadable), tcp->obj, xsResult);
		xsEndHost(the);
	}

	if (tcp->buffers && tcp->error)
		;		// error delivered after receive buffers empty
	else if (!tcp->error) {
		if ((triggered & tcp->triggerable & kTCPWritable) && tcp->skt) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, tcp_sndbuf(tcp->skt));
				xsCallFunction1(xsReference(tcp->onWritable), tcp->obj, xsResult);
			xsEndHost(the);
		}
	}
	else if (tcp->triggerable & kTCPError) {
		xsBeginHost(the);
			xsCallFunction0(xsReference(tcp->onError), tcp->obj);
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

	removeTCPCallbacks(tcp);
	tcp->skt = NULL;		// "pcb is already freed when this callback is called"
	if (kTCPError & tcp->triggerable)
		tcpTrigger(tcp, kTCPError);
}

err_t tcpReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, err_t err)
{
	TCP tcp = arg;
	TCPBuffer buffer;

	if ((NULL == pb) || (ERR_OK != err)) {		//@@ when is err set here?
		removeTCPCallbacks(tcp);
#if ESP32
//@@		tcp->skt = NULL;			// no close on socket if disconnected.
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

	modInstrumentationAdjust(NetworkBytesRead, buffer->bytes);

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
#if TCP_ATOMICS
	uint8_t triggered = modAtomicsFetchOr(&tcp->triggered, trigger);
#else
	uint8_t triggered;

	builtinCriticalSectionBegin();
	triggered = tcp->triggered;
	tcp->triggered |= trigger;
	builtinCriticalSectionEnd();
#endif
	if (!triggered) {
		tcpHold(tcp);
		modMessagePostToMachine(tcp->the, NULL, 0, tcpDeliver, tcp);
	}
}

void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	TCP tcp = it;

	if (tcp->triggerable) {
		if (tcp->onReadable)
			(*markRoot)(the, tcp->onReadable);
		if (tcp->onWritable)
			(*markRoot)(the, tcp->onWritable);
		if (tcp->onError)
			(*markRoot)(the, tcp->onError);
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
	uint8_t			triggered;
	xsMachine		*the;
	xsSlot			*onReadable;
//	xsSlot			*onError;
};
typedef struct ListenerRecord ListenerRecord;
typedef struct ListenerRecord *Listener;

static err_t listenerAccept(void *arg, struct tcp_pcb *skt, err_t err);
static void listenerDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void listenerTrigger(Listener listener, uint8_t trigger);

static void xs_listener_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static const xsHostHooks ICACHE_RODATA_ATTR xsListenerHooks = {
	xs_listener_destructor_,
	xs_listener_mark,
	NULL
};

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
	xsSlot *onReadable;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}
	//@@ address

	onReadable = builtinGetCallback(the, xsID_onReadable);
	// hasOnError

	if (kIOFormatSocketTCP != builtinInitializeFormat(the, kIOFormatSocketTCP))
		xsRangeError("unimplemented");

	skt = tcp_new();
	if (!skt)
		xsUnknownError("no socket");

	skt->so_options |= SOF_REUSEADDR;
	if (tcp_bind_safe(skt, &address, port)) {
		tcp_close_safe(skt);
		xsUnknownError("socket bind");
	}

	skt = tcp_listen_safe(skt);
	if (!skt) {
		tcp_close_safe(skt);
		xsRangeError("no memory");
	}

	listener = c_calloc(1, sizeof(ListenerRecord));
	if (!listener) {
		tcp_close_safe(skt);
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

	listener->onReadable = onReadable;
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsListenerHooks);
}

void xs_listener_destructor_(void *data)
{
	Listener listener = data;
	if (!data) return;

	tcp_accept(listener->skt, NULL);
	tcp_close_safe(listener->skt);

	while (listener->pending) {
		ListenerPending pending = listener->pending;
		listener->pending = pending->next;
		tcp_close_safe(pending->skt);
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

	if (NULL == listener->pending) {
		xsDebugger();
		xsCall0(xsArg(0), xsID_close);
		return;
	}

	builtinCriticalSectionBegin();
	pending = listener->pending;
	listener->pending = pending->next;
	builtinCriticalSectionEnd();

	xsResult = xsArg(0);
	tcp = xsmcGetHostDataValidate(xsArg(0), (void *)&xsTCPHooks);

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
	uint8_t triggered;

	builtinCriticalSectionBegin();
	triggered = listener->triggered;
	listener->triggered = 0;
	builtinCriticalSectionEnd();

	if ((triggered & kListenerReadable) && listener->pending) {
		ListenerPending walker;
		int sockets = 0;

		for (walker = listener->pending; NULL != walker; walker = walker->next)
			sockets++;

		xsBeginHost(the);
			xsmcSetInteger(xsResult, sockets);
			xsCallFunction1(xsReference(listener->onReadable), listener->obj, xsResult);
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
	//@@ also install error handler to know if pending socket disconnects?

	builtinCriticalSectionBegin();
	walker = listener->pending;
	if (NULL == walker)
		listener->pending = pending;
	else {
		while (walker->next)
			walker = walker->next;
		walker->next = pending;
	}
	builtinCriticalSectionEnd();

	if (listener->onReadable)
		listenerTrigger(listener, kListenerReadable);

	return ERR_OK;
}

void listenerTrigger(Listener listener, uint8_t trigger)
{
	uint8_t triggered;

	builtinCriticalSectionBegin();
	triggered = listener->triggered;
	listener->triggered |= trigger;
	builtinCriticalSectionEnd();

	if (!triggered)
		modMessagePostToMachine(listener->the, NULL, 0, listenerDeliver, listener);
}

void xs_listener_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Listener listener = it;

	if (listener->onReadable)
		(*markRoot)(the, listener->onReadable);
}
