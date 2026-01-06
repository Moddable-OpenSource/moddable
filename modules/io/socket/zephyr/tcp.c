/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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
	TCP socket - uing Zephyr low-level net_context

	To do:
		find a way to eliminate "pending" workaround on write

		deliver details to onError (disconnect, etc)
		allow collection (xsForget) once error /  disconnect
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"		// platform support
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values
#include "builtinCommon.h"
#include "modTImer.h"

#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>

struct TCPBufferRecord {
	struct TCPBufferRecord *next;
	struct net_pkt *pkt;
	uint16_t		bytes;			// unread bytes in pkt
	uint16_t		total;			// total bytes in pkt
};
typedef struct TCPBufferRecord TCPBufferRecord;
typedef struct TCPBufferRecord *TCPBuffer;

enum {
	kTCPWritable = 1 << 0,
	kTCPReadable = 1 << 1,
	kTCPError = 1 << 2
};

struct TCPRecord {
	struct net_context *context;
	xsSlot			obj;
	TCPBuffer		buffers;
	atomic_t			useCount;
	atomic_t			triggered;
	uint8_t			triggerable;
	uint8_t			error;
	uint8_t			format;
	uint8_t			ready;
	int 				sendBufferSize;
	int				sendBytesInFlight;

	uint8_t			*pending;
	uint8_t			*pendingBuffer;
	uint32_t			pendingBytes;
	modTimer			pendingTimer;

	xsMachine		*the;
	xsSlot			*onReadable;
	xsSlot			*onWritable;
	xsSlot			*onError;
};
typedef struct TCPRecord TCPRecord;
typedef struct TCPRecord *TCP;

typedef struct {
	TCP tcp;
	int byteLength;
} TCPWriteRecord, *TCPWrite;

static void removeTCPCallbacks(TCP tcp);
static void tcpHold(TCP tcp);
static void tcpRelease(TCP tcp);

static void tcpConnect(struct net_context *context, int status, void *user_data);
static void tcpReceive(struct net_context *context, struct net_pkt *pkt, union net_ip_header *ip_hdr, union net_proto_header *proto_hdr, int status, void *user_data);
static void tcpSent(struct net_context *context, int status, void *user_data);
static void tcpTrigger(TCP tcp, uint8_t trigger);
static void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void doClose(xsMachine *the, xsSlot *instance);
static void tcpTrySend(modTimer timer, void *refcon, int refconSize);

static const xsHostHooks ICACHE_RODATA_ATTR xsTCPHooks = {
	xs_tcp_destructor,
	xs_tcp_mark,
	NULL
};

void xs_tcp_constructor(xsMachine *the)
{
	TCP tcp = C_NULL;
	uint8_t create = xsmcArgc > 0;
	uint8_t connect = 0, triggerable = 0, triggered = 0, nodelay = 0, format = kIOFormatBuffer;
	xsSlot *onReadable, *onWritable, *onError;
	struct net_context *context = C_NULL;;
	struct sockaddr_in remote;
	int result;

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
			xsmcGet(xsVar(0), xsArg(0), xsID_from);
			tcp = xsmcGetHostDataValidate(xsVar(0), (void *)&xsTCPHooks);
			context = tcp->context;
			if (!context)
				xsUnknownError("invalid from");

			if ((triggerable & kTCPWritable) && (tcp->sendBufferSize > tcp->sendBytesInFlight))
				triggered |= kTCPWritable;
			if ((triggerable & kTCPReadable) && tcp->buffers)
				triggered |= kTCPReadable;

			xsForget(tcp->obj);
			xsmcSetHostData(xsVar(0), NULL);
			xsmcSetHostDestructor(xsVar(0), NULL);
		}
		else {
			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			int port = builtinGetSignedInteger(the, &xsVar(0)); 
			if ((port < 0) || (port > 65535))
				xsRangeError("invalid port");

			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			remote.sin_family = AF_INET;
			remote.sin_port = htons(port);
			if (net_addr_pton(AF_INET, xsmcToString(xsVar(0)), &remote.sin_addr) < 0)
				xsRangeError("invalid address");

			result = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &context);
			if (result < 0)
				xsRangeError("no socket");

//@@			skt->so_options |= SOF_REUSEADDR;
			struct sockaddr_in local = {0};
			local.sin_family = AF_INET;
			local.sin_port = 0;
			local.sin_addr.s_addr = INADDR_ANY;
			result = net_context_bind(context, (struct sockaddr *)&local, sizeof(local));
			if (result < 0) {
				net_context_put(context);
				xsUnknownError("bind failed");
			}
		}
	}

	if (C_NULL == tcp) {
		tcp = c_calloc(1, sizeof(TCPRecord));
		if (!tcp) {
			net_context_put(context);
			xsRangeError("no memory");
		}

		modInstrumentationAdjust(NetworkSockets, +1);

		tcp->the = the;
		tcp->format = format;
		atomic_set(&tcp->useCount, 0);
		tcpHold(tcp);

		tcp->sendBufferSize = 2048;
		tcp->context = context;

		connect = true;
	}

	tcp->obj = xsThis;
	xsRemember(tcp->obj);
	xsmcSetHostData(xsThis, tcp);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsTCPHooks);

	if (!create)
		return;

	net_context_set_option(context, NET_OPT_SNDBUF, &tcp->sendBufferSize, sizeof(tcp->sendBufferSize));

	if (net_context_get_option(context, NET_OPT_SNDBUF, &tcp->sendBufferSize, C_NULL) < 0)
		xsUnknownError("can't get NET_OPT_SNDBUF");

#if 0
	if (nodelay) {
		if (2 == nodelay)
			tcp_nagle_disable(skt);
		else
			tcp_nagle_enable(skt);
	}
#endif

	builtinInitializeTarget(the);

	result = net_context_recv(context, tcpReceive, K_NO_WAIT, tcp);
	if (result < 0)
		xsUnknownError("error");

	tcp->triggerable = triggerable;
	if (triggerable) {
		tcp->onReadable = onReadable;
		tcp->onWritable = onWritable;
		tcp->onError = onError;
	}

	if (connect) {
		if (net_context_connect(context, (struct sockaddr *)&remote, sizeof(remote), tcpConnect, K_FOREVER, tcp))
			xsUnknownError("connect error");
	}
	else if (triggered)
		tcpTrigger(tcp, triggered);
}

void xs_tcp_destructor(void *data)
{
	TCP tcp = data;
	if (!tcp) return;

	if (tcp->context) {
		removeTCPCallbacks(tcp);

		net_context_put(tcp->context);
	}

	while (tcp->buffers) {
		TCPBuffer buffer = tcp->buffers;
		tcp->buffers = buffer->next;
		net_pkt_unref(buffer->pkt);
		c_free(buffer);
	}

	if (tcp->pending)
		c_free(tcp->pending);

	if (tcp->pendingTimer)
		modTimerRemove(tcp->pendingTimer);

	c_free(data);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void removeTCPCallbacks(TCP tcp)
{
	tcp->triggerable &= ~kTCPWritable;
}

void doClose(xsMachine *the, xsSlot *instance)
{
	TCP tcp = xsmcGetHostData(*instance);
	if (tcp && xsmcGetHostDataValidate(*instance, (void *)&xsTCPHooks)) {
		tcp->ready = false;

		removeTCPCallbacks(tcp);
		tcp->triggerable = 0;

		xsmcSetHostData(*instance, NULL);
		xsForget(tcp->obj);
		xsmcSetHostDestructor(*instance, NULL);
		atomic_set(&tcp->triggered, 0);
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
			xsmcSetInteger(xsResult, requested);
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

	//@@ not sure we need a loop... that was mostly about lwip fragments
	while (requested) {
		int use = requested;
		TCPBuffer buffer = tcp->buffers;

		if (use > buffer->bytes)
			use = buffer->bytes;

		if (0 != net_pkt_read(buffer->pkt, out, use))
			xsUnknownError("net_pkt_read failed");

		out += use;
		requested -= use;
		buffer->bytes -= use;

		if (0 == buffer->bytes) {
			tcp->buffers = buffer->next;
			net_context_update_recv_wnd(tcp->context, buffer->total);
			net_pkt_unref(buffer->pkt);
			c_free(buffer);
			if (NULL == tcp->buffers) {
				if (tcp->error)
					tcpTrigger(tcp, kTCPError);
				break;
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

	if (!tcp->ready || tcp->pending)
		xsUnknownError("not ready");

	if (tcp->error || !tcp->context)
		return;		//@@

	if (kIOFormatBuffer == tcp->format)
		xsmcGetBufferReadable(xsArg(0), &buffer, &needed);
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}
	int available = tcp->sendBufferSize - tcp->sendBytesInFlight;
	if ((needed > available) || tcp->pending)
		xsUnknownError("would block");

	TCPWrite tw = c_malloc(sizeof(TCPWriteRecord));
	if (!tw)
		xsUnknownError("no memory");
	tw->tcp = tcp;
	tw->byteLength = needed;

	int result = net_context_send(tcp->context, buffer, needed, tcpSent, K_NO_WAIT, tw);
	if (-EAGAIN == result)
		result = 0;
	else if (result < 0) {
		xsTrace("tcp write failed\n");
		tcpTrigger(tcp, kTCPError);		//@@ correct!?!
		return;
	}

	if (result != needed) {
		tcp->pending = c_malloc(needed - result);
		if (!tcp->pending)
			xsUnknownError("no memory");		//@@ unrecoverable
		c_memcpy(tcp->pending, result + (uint8_t *)buffer, needed - result);
		tcp->pendingBuffer = tcp->pending;
		tcp->pendingBytes = needed - result;

		tcp->pendingTimer = modTimerAdd(100, 50, tcpTrySend, &tcp, sizeof(tcp));
	}

	tcp->sendBytesInFlight += needed;
	available -= needed;

	modInstrumentationAdjust(NetworkBytesWritten, needed);
	xsmcSetInteger(xsResult, tcp->pendingBytes ? 0 : available);
}

void xs_tcp_get_remoteAddress(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	struct sockaddr_in *remote = (struct sockaddr_in *)&tcp->context->remote;
	xsResult = xsStringBuffer(NULL, NET_IPV4_ADDR_LEN);
	net_addr_ntop(AF_INET, &remote->sin_addr, xsmcToString(xsResult), INET_ADDRSTRLEN);
}

void xs_tcp_get_remotePort(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	struct sockaddr_in *remote = (struct sockaddr_in *)&tcp->context->remote;
	xsmcSetInteger(xsResult, ntohs(remote->sin_port));
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
	atomic_inc(&tcp->useCount);
}

void tcpRelease(TCP tcp)
{
	if (1 == atomic_dec(&tcp->useCount))
		xs_tcp_destructor(tcp);
}

void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	TCP tcp = refcon;
	uint8_t triggered = atomic_set(&tcp->triggered, 0);
	if (triggered & kTCPError) {
		triggered &= ~kTCPWritable;
		tcp->error = true;
		tcp->ready = false;
	}

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
		if ((triggered & tcp->triggerable & kTCPWritable) && tcp->context && ((tcp->sendBufferSize - tcp->sendBytesInFlight) > 0) && (0 == tcp->pendingBytes)) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, tcp->sendBufferSize - tcp->sendBytesInFlight);
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

void tcpConnect(struct net_context *context, int status, void *user_data)
{
	TCP tcp = user_data;
	tcp->ready = status >= 0;
	if (!tcp->ready)
		tcp->error = status;
	tcpTrigger(tcp, tcp->ready ? kTCPWritable : kTCPError);
}

void tcpReceive(struct net_context *context, struct net_pkt *pkt, union net_ip_header *ip_hdr, union net_proto_header *proto_hdr, int status, void *user_data)
{
	TCP tcp = user_data;
	TCPBuffer buffer;

	if ((NULL == pkt) || (status < 0)) {
		tcp->error = status;
		removeTCPCallbacks(tcp);
		tcpTrigger(tcp, kTCPError);
		return;
	}

// #ifdef mxDebug
// 	if (fxInNetworkDebugLoop(tcp->the))
// 		return;
// #endif

	buffer = c_malloc(sizeof(TCPBufferRecord));
	if (!buffer) {
		net_pkt_unref(buffer->pkt);
		return;		//@@ are we toast here?
	}

	buffer->next = NULL;
	buffer->pkt = pkt;
	buffer->bytes = net_pkt_remaining_data(pkt);
	buffer->total = buffer->bytes;

	modInstrumentationAdjust(NetworkBytesRead, buffer->bytes);

	if (tcp->buffers) {
		TCPBuffer walker;

		for (walker = tcp->buffers; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	else
		tcp->buffers = buffer;

	net_context_update_recv_wnd(tcp->context, -buffer->total);

	if (tcp->triggerable & kTCPReadable)
		tcpTrigger(tcp, kTCPReadable);
}

void tcpSent(struct net_context *context, int status, void *user_data)
{
	TCPWrite tw = user_data;
	if (!tw) return;
	TCP tcp = tw->tcp;
	tcp->sendBytesInFlight -= tw->byteLength;
	c_free(tw);

	tcpTrigger(tcp, (status < 0) ? kTCPError : kTCPWritable);
}

void tcpTrigger(TCP tcp, uint8_t trigger)
{
	uint8_t triggered = atomic_or(&tcp->triggered, trigger);
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

void tcpTrySend(modTimer timer, void *refcon, int refconSize)
{
	TCP tcp = *(TCP *)refcon;
	int result = net_context_send(tcp->context, tcp->pendingBuffer, tcp->pendingBytes, C_NULL, K_NO_WAIT, C_NULL);
	if (result <= 0) {
		if (-EAGAIN == result)
			result = 0;
		if (result)
			tcpTrigger(tcp, kTCPError);
		return;
	}

	tcp->pendingBuffer += result;
	tcp->pendingBytes -= result;
	if (tcp->pendingBytes)
		return;

	c_free(tcp->pending);
	tcp->pending = tcp->pendingBuffer = C_NULL;

	modTimerRemove(tcp->pendingTimer);
	tcp->pendingTimer = C_NULL;

	tcpTrigger(tcp, kTCPWritable);
}

/*
	Listener
*/

struct ListenerPendingRecord {
	struct ListenerPendingRecord *next;
	struct net_context *context;
};
typedef struct ListenerPendingRecord ListenerPendingRecord;
typedef struct ListenerPendingRecord *ListenerPending;

struct ListenerRecord {
	struct net_context *context;
	xsSlot			obj;
	ListenerPending	pending;
	atomic_t			triggered;
	xsMachine		*the;
	xsSlot			*onReadable;
//	xsSlot			*onError;
};
typedef struct ListenerRecord ListenerRecord;
typedef struct ListenerRecord *Listener;

static void listenerAccept(struct net_context *context, struct sockaddr *addr, socklen_t addrlen, int status, void *user_data);
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
	struct net_context *context;
	int port = 0;
	xsSlot *onReadable;

	CHECK_NETWORK_SAFE();

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = builtinGetSignedInteger(the, &xsVar(0)); 
		if ((port < 0) || (port > 65535))
			xsRangeError("invalid port");
	}

	onReadable = builtinGetCallback(the, xsID_onReadable);
	// hasOnError

	if (kIOFormatSocketTCP != builtinInitializeFormat(the, kIOFormatSocketTCP))
		xsRangeError("unimplemented");

	if (net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &context) < 0)
		xsUnknownError("no socket");

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};
	if (net_context_bind(context, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		net_context_put(context);
		xsUnknownError("socket bind");
	}

	if (net_context_listen(context, 5) < 0) {
		net_context_put(context);
		xsRangeError("can't listen");
	}

	listener = c_calloc(1, sizeof(ListenerRecord));
	if (!listener) {
		net_context_put(context);
		xsRangeError("no memory");
	}

	modInstrumentationAdjust(NetworkSockets, +1);

	xsmcSetHostData(xsThis, listener);
	listener->obj = xsThis;
	xsRemember(listener->obj);
	listener->context = context;
	listener->the = the;

	builtinInitializeTarget(the);

	net_context_accept(context, listenerAccept, K_FOREVER, listener);

	listener->onReadable = onReadable;
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsListenerHooks);
}

void xs_listener_destructor_(void *data)
{
	Listener listener = data;
	if (!data) return;

	net_context_put(listener->context);

	while (listener->pending) {
		ListenerPending pending = listener->pending;
		listener->pending = pending->next;
		net_context_put(pending->context);
		c_free(pending);
	}

	c_free(listener);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_listener_close_(xsMachine *the)
{
	Listener listener = xsmcGetHostData(xsThis);
	if (listener && xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks)) {
		xsForget(listener->obj);
		xs_listener_destructor_(listener);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_listener_read(xsMachine *the)
{
	Listener listener = xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks);
	ListenerPending pending;
	TCP tcp;

	if (NULL == listener->pending)
		return;

	builtinCriticalSectionBegin();
	pending = listener->pending;
	listener->pending = pending->next;
	builtinCriticalSectionEnd();

	xsResult = xsArg(0);
	tcp = xsmcGetHostDataValidate(xsArg(0), (void *)&xsTCPHooks);

	tcp->context = pending->context;
	c_free(pending);

	tcp->ready = true;
}

void xs_listener_get_port(xsMachine *the)
{
	Listener listener = xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks);
	struct sockaddr_in *local = (struct sockaddr_in *)&listener->context->local;

	xsmcSetInteger(xsResult, ntohs(local->sin_port));
}

void listenerDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Listener listener = refcon;
	uint8_t triggered = atomic_set(&listener->triggered, 0);

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

static void listenerAccept(struct net_context *context, struct sockaddr *addr, socklen_t addrlen, int status, void *user_data)
{
	Listener listener = user_data;
	ListenerPending pending, walker;

	net_context_accept(listener->context, listenerAccept, K_FOREVER, listener);

// #ifdef mxDebug
// 	if (fxInNetworkDebugLoop(listener->the)) {
// 		net_context_put(context);
// 		return;
// 	}
// #endif

	pending = c_malloc(sizeof(ListenerPendingRecord));
	if (C_NULL == pending) {
		net_context_put(context);
		//@@ listener onError
		return;
	}
	pending->next = NULL;
	pending->context = context;

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
}

void listenerTrigger(Listener listener, uint8_t trigger)
{
	uint8_t triggered = atomic_or(&listener->triggered, trigger);
	if (!triggered)
		modMessagePostToMachine(listener->the, NULL, 0, listenerDeliver, listener);
}

void xs_listener_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Listener listener = it;

	if (listener->onReadable)
		(*markRoot)(the, listener->onReadable);
}
