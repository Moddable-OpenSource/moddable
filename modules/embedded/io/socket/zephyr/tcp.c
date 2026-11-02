/*
 * Copyright (c) 2019-2026  Moddable Tech, Inc.
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
		deliver details to onError (disconnect, etc)
		allow collection (xsForget) once error /  disconnect
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"		// platform support
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values
#include "builtinCommon.h"

#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>

#ifndef MODDEF_TCP_SENDBUFFER
	#define MODDEF_TCP_SENDBUFFER (2048)
#endif
#ifndef MODDEF_TCP_RECEIVEBUFFER		// received bytes the application has not read for which the receive window is reopened as they arrive: by default, Zephyr's receive window (tcp_rx_window in tcp.c)
	#if CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE
		#define MODDEF_TCP_RECEIVEBUFFER (CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE)
	#elif defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
		#define MODDEF_TCP_RECEIVEBUFFER ((CONFIG_NET_BUF_RX_COUNT * CONFIG_NET_BUF_DATA_SIZE) / 3)
	#else
		#define MODDEF_TCP_RECEIVEBUFFER (CONFIG_NET_PKT_BUF_RX_DATA_POOL_SIZE / 3)
	#endif
#endif

// Zephyr's TCP gives this semaphore when the connection's send window opens, and when the connection closes; its sockets wait on it (sockets_inet.c). The declaration is in subsys/net/ip/tcp_internal.h, which is not public.
extern struct k_sem *net_tcp_tx_sem_get(struct net_context *context);

struct TCPBufferRecord {
	struct TCPBufferRecord *next;
	struct net_pkt *pkt;
	uint16_t		bytes;			// unread bytes in pkt
};
typedef struct TCPBufferRecord TCPBufferRecord;
typedef struct TCPBufferRecord *TCPBuffer;

enum {
	kTCPWritable = 1 << 0,
	kTCPReadable = 1 << 1,
	kTCPError = 1 << 2,
	kTCPOutput = 1 << 3,			// written bytes to flush
	kTCPSendable = 1 << 4		// the wait for send space ended
};

struct TCPRecord {
	struct net_context *context;
	xsSlot			obj;
	TCPBuffer		buffers;		// appended by tcpReceive on the network thread, consumed by the XS thread: changed only in a critical section, as udp.c does
	uint32_t			unread;			// bytes in buffers: changed only in a critical section
	uint32_t			deferred;		// bytes received for which the receive window is not yet reopened: changed only in a critical section
	atomic_t			useCount;
	atomic_t			triggered;
	uint8_t			triggerable;
	uint8_t			error;
	uint8_t			format;
	uint8_t			ready;
	int 				sendBufferSize;

	uint8_t			*output;			// written bytes Zephyr's TCP has not queued yet: sendBufferSize bytes, while in use
	uint32_t			outputBytes;
	uint8_t			waiting;			// for send space: holds tcp until tcpDeliver sees kTCPSendable
	struct k_work_poll	sendable;
	struct k_poll_event	sendableEvent;

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

static TCP tcpNew(xsMachine *the, struct net_context *context);
static void tcpConnect(struct net_context *context, int status, void *user_data);
static void tcpReceive(struct net_context *context, struct net_pkt *pkt, union net_ip_header *ip_hdr, union net_proto_header *proto_hdr, int status, void *user_data);
static void tcpReopen(TCP tcp);
static void tcpWaitSendable(TCP tcp, uint8_t buffers);
static void tcpSendable(struct k_work *work);
static uint8_t tcpWindowOpen(TCP tcp);
static void tcpFlush(TCP tcp);
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

		if (xsmcGet(xsVar(0), xsArg(0), xsID_noDelay))
			nodelay = xsmcToBoolean(xsVar(0)) ? 2 : 1;

		if (xsmcGet(xsVar(0), xsArg(0), xsID_from)) {
			tcp = xsmcGetHostDataValidate(xsVar(0), (void *)&xsTCPHooks);
			context = tcp->context;
			if (!context)
				xsUnknownError("invalid from");

			if ((triggerable & kTCPWritable) && !tcp->outputBytes)
				triggered |= kTCPWritable;

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
			net_context_ref(context);		// the stack's reference, released when its connection is: it uses the context through the closing handshake, after net_context_put releases ours

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
		tcp = tcpNew(the, context);
		if (!tcp) {
			net_context_put(context);
			xsRangeError("no memory");
		}
		tcp->format = format;

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

	tcp->triggerable = triggerable;		// before net_context_recv: from then, tcpReceive may run on the network thread
	if (triggerable) {
		tcp->onReadable = onReadable;
		tcp->onWritable = onWritable;
		tcp->onError = onError;
	}

	result = net_context_recv(context, tcpReceive, K_NO_WAIT, tcp);		// an accepted connection's tcpReceive is already registered, with this tcp
	if (result < 0)
		xsUnknownError("error");

	if ((triggerable & kTCPReadable) && tcp->buffers)		// after triggerable is set: tcpReceive triggers what arrives from then
		triggered |= kTCPReadable;
	if (tcp->error)		// an accepted connection may have closed before it was read from the listener
		triggered |= kTCPError;

	if (connect) {
		if (net_context_connect(context, (struct sockaddr *)&remote, sizeof(remote), tcpConnect, K_FOREVER, tcp))
			xsUnknownError("connect error");
	}
	else if (triggered)
		tcpTrigger(tcp, triggered);
}

TCP tcpNew(xsMachine *the, struct net_context *context)
{
	TCP tcp = c_calloc(1, sizeof(TCPRecord));
	if (!tcp)
		return NULL;

	modInstrumentationAdjust(NetworkSockets, +1);

	tcp->the = the;
	tcp->format = kIOFormatBuffer;
	atomic_set(&tcp->useCount, 0);
	tcpHold(tcp);
	k_work_poll_init(&tcp->sendable, tcpSendable);

	tcp->sendBufferSize = MODDEF_TCP_SENDBUFFER;
	tcp->context = context;

	return tcp;
}

void xs_tcp_destructor(void *data)
{
	TCP tcp = data;
	if (!tcp) return;

	if (tcp->waiting)		// only when the machine is torn down: otherwise the wait holds tcp
		k_work_poll_cancel(&tcp->sendable);

	if (tcp->context) {
		removeTCPCallbacks(tcp);

		net_context_put(tcp->context);
	}

	builtinCriticalSectionBegin();
	TCPBuffer buffers = tcp->buffers;
	tcp->buffers = NULL;
	builtinCriticalSectionEnd();
	while (buffers) {
		TCPBuffer buffer = buffers;
		buffers = buffer->next;
		net_pkt_unref(buffer->pkt);
		c_free(buffer);
	}

	if (tcp->output)
		c_free(tcp->output);

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
		atomic_and(&tcp->triggered, kTCPSendable);		// a wait that already ended still releases its hold in tcpDeliver
		if (tcp->waiting && (0 == k_work_poll_cancel(&tcp->sendable))) {
			tcp->waiting = false;
			tcpRelease(tcp);
		}
		tcpRelease(tcp);
	}
}

void xs_tcp_close(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);
	if (tcp && xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks) && tcp->ready && tcp->outputBytes)
		tcpFlush(tcp);		// what Zephyr's TCP queues now is sent; the rest is lost
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

	int consumed = requested;

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
			builtinCriticalSectionBegin();		// tcpReceive may be appending to this buffer
			tcp->buffers = buffer->next;
			builtinCriticalSectionEnd();
			net_pkt_unref(buffer->pkt);
			c_free(buffer);
			if (NULL == tcp->buffers) {
				if (tcp->error)
					tcpTrigger(tcp, kTCPError);
				break;
			}
		}
	}

	builtinCriticalSectionBegin();
	tcp->unread -= consumed;
	builtinCriticalSectionEnd();
	tcpReopen(tcp);		// reopens the receive window for bytes held back while unread ones filled MODDEF_TCP_RECEIVEBUFFER

	if (kIOFormatNumber == tcp->format)
		xsmcSetInteger(xsResult, value);
}

void xs_tcp_write(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);
	xsUnsignedValue needed;
	void *buffer;
	uint8_t value;

	if (!tcp->ready)
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
	if (needed > (tcp->sendBufferSize - tcp->outputBytes))
		xsUnknownError("would block");

	if (!tcp->output) {
		tcp->output = c_malloc(tcp->sendBufferSize);
		if (!tcp->output)
			xsUnknownError("no memory");
	}
	c_memcpy(tcp->output + tcp->outputBytes, buffer, needed);
	tcp->outputBytes += needed;
	tcpTrigger(tcp, kTCPOutput);		// tcpDeliver flushes what is written this turn together

	modInstrumentationAdjust(NetworkBytesWritten, needed);
	xsmcSetInteger(xsResult, tcp->sendBufferSize - tcp->outputBytes);
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

	if (triggered & kTCPSendable) {
		tcp->waiting = false;
		tcpRelease(tcp);		// the wait's hold: this delivery holds tcp too
	}

	if ((triggered & (kTCPOutput | kTCPSendable)) && tcp->ready && tcp->outputBytes && !tcp->waiting) {
		tcpFlush(tcp);
		if (!tcp->outputBytes)		// all written bytes are with Zephyr's TCP: onWritable refills while they are sent
			triggered |= kTCPWritable;
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
		if ((triggered & tcp->triggerable & kTCPWritable) && tcp->context && !tcp->outputBytes) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, tcp->sendBufferSize);
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
		net_pkt_unref(pkt);
		return;		//@@ are we toast here?
	}

	buffer->next = NULL;
	buffer->pkt = pkt;
	buffer->bytes = net_pkt_remaining_data(pkt);

	modInstrumentationAdjust(NetworkBytesRead, buffer->bytes);

	builtinCriticalSectionBegin();		// the XS thread may be removing the head
	if (tcp->buffers) {
		TCPBuffer walker;

		for (walker = tcp->buffers; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	else
		tcp->buffers = buffer;
	tcp->unread += buffer->bytes;
	tcp->deferred += buffer->bytes;
	builtinCriticalSectionEnd();

	tcpReopen(tcp);		// Zephyr's TCP shrinks the window as it delivers data, and delays its ACK while the window is short

	if (tcp->triggerable & kTCPReadable)
		tcpTrigger(tcp, kTCPReadable);
}

void tcpReopen(TCP tcp)		// network thread (tcpReceive) or XS thread (xs_tcp_read)
{
	int32_t amount;

	builtinCriticalSectionBegin();
	amount = MODDEF_TCP_RECEIVEBUFFER - (int32_t)(tcp->unread - tcp->deferred);		// room for more unread bytes with the window reopened
	if (amount > (int32_t)tcp->deferred)
		amount = tcp->deferred;
	if (amount > 0)
		tcp->deferred -= amount;
	builtinCriticalSectionEnd();

	if (amount > 0)		// Zephyr's TCP sends the window update (and the ACK it delayed) at once: tcp_in delivers data after updating the ACK and releasing the connection lock
		net_context_update_recv_wnd(tcp->context, amount);
}

void tcpWaitSendable(TCP tcp, uint8_t buffers)
{
	int result;

	if (tcp->waiting)
		return;

	tcp->waiting = true;
	tcpHold(tcp);
	if (buffers || !tcp->context->tcp)		// out of network buffers (or not Zephyr's native TCP): try again shortly
		result = k_work_poll_submit(&tcp->sendable, &tcp->sendableEvent, 0, K_MSEC(10));
	else {
		k_poll_event_init(&tcp->sendableEvent, K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, net_tcp_tx_sem_get(tcp->context));
		result = k_work_poll_submit(&tcp->sendable, &tcp->sendableEvent, 1, K_SECONDS(1));		// the timeout is only a safety net
	}
	if (result)
		tcpTrigger(tcp, kTCPSendable);
}

void tcpSendable(struct k_work *work)		// on the system work queue
{
	struct k_work_poll *sendable = CONTAINER_OF(work, struct k_work_poll, work);
	tcpTrigger(CONTAINER_OF(sendable, TCPRecord, sendable), kTCPSendable);
}

uint8_t tcpWindowOpen(TCP tcp)
{
	return !tcp->context->tcp || k_sem_count_get(net_tcp_tx_sem_get(tcp->context));
}

void tcpFlush(TCP tcp)
{
	if (!tcpWindowOpen(tcp)) {
		tcpWaitSendable(tcp, false);
		return;
	}

	int result = net_context_send(tcp->context, tcp->output, tcp->outputBytes, C_NULL, K_NO_WAIT, C_NULL);
	if (result < 0) {
		if ((-EAGAIN == result) || (-ENOBUFS == result))		// window full, network buffers exhausted: both pass
			tcpWaitSendable(tcp, -ENOBUFS == result);
		else
			tcpTrigger(tcp, kTCPError);
		return;
	}

	tcp->outputBytes -= result;
	if (tcp->outputBytes) {		// Zephyr's TCP queues only what its send window holds
		c_memmove(tcp->output, tcp->output + result, tcp->outputBytes);
		tcpWaitSendable(tcp, false);
	}
	else {
		c_free(tcp->output);
		tcp->output = C_NULL;
	}
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

/*
	Listener
*/

struct ListenerPendingRecord {
	struct ListenerPendingRecord *next;
	TCP					tcp;		// created at accept, so that tcpReceive has data that arrives before the connection is read
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

	if (xsmcGet(xsVar(0), xsArg(0), xsID_port)) {
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
	net_context_ref(context);		// the stack's reference, as for a connection

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
		tcpRelease(pending->tcp);
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
	TCP tcp, unused;

	if (NULL == listener->pending)
		return;

	xsResult = xsNewFunction0(xsArg(0));
	unused = xsmcGetHostDataValidate(xsResult, (void *)&xsTCPHooks);

	builtinCriticalSectionBegin();
	pending = listener->pending;
	listener->pending = pending->next;
	builtinCriticalSectionEnd();

	tcp = pending->tcp;
	c_free(pending);

	xsmcSetHostData(xsResult, tcp);		// the instance takes the record created at accept, as the from constructor takes it from this instance
	tcp->obj = xsResult;
	xsRemember(tcp->obj);
	xsForget(unused->obj);
	tcpRelease(unused);
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
	TCP tcp;

	if (status < 0)		// the network went down: context is the listener's own
		return;

	net_context_ref(context);		// the stack's reference, as for a connection
	net_context_accept(listener->context, listenerAccept, K_FOREVER, listener);

// #ifdef mxDebug
// 	if (fxInNetworkDebugLoop(listener->the)) {
// 		net_context_put(context);
// 		return;
// 	}
// #endif

	pending = c_malloc(sizeof(ListenerPendingRecord));
	tcp = pending ? tcpNew(listener->the, context) : NULL;
	if (C_NULL == tcp) {
		c_free(pending);
		net_context_put(context);
		//@@ listener onError
		return;
	}
	tcp->ready = true;
	if (net_context_recv(context, tcpReceive, K_NO_WAIT, tcp) < 0) {		// at once, as Zephyr's sockets do: Zephyr's TCP acknowledges and discards data that arrives while a connection has no receive callback
		tcpRelease(tcp);
		c_free(pending);
		return;
	}
	pending->next = NULL;
	pending->tcp = tcp;

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
