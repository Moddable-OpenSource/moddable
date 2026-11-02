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
#include "modTimer.h"

#ifndef MODDEF_TCP_OUTPUTBUFFER_IDLE
	#define MODDEF_TCP_OUTPUTBUFFER_IDLE (20)
#endif
#ifndef MODDEF_TCP_OUTPUTBUFFER_MAX
	#define MODDEF_TCP_OUTPUTBUFFER_MAX (TCP_MSS)
#endif

struct TCPOutBufferRecord {
	struct TCPOutBufferRecord *next;
	uint16_t		size;			// capacity
	uint16_t		bytes;			// filled
	uint8_t			data[];
};
typedef struct TCPOutBufferRecord TCPOutBufferRecord;
typedef struct TCPOutBufferRecord *TCPOutBuffer;

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
	TCPOutBuffer	out;
	TCPOutBuffer	outTail;
	uint32_t		unwritten;
	uint32_t		unacked;
	uint32_t		writeRemaining;
	modTimer		timer;
	uint8_t			more;
	uint8_t			triggerable;
	uint8_t			triggered;
	int8_t			useCount;
	uint8_t			error;
	uint8_t			format;
	uint8_t			ready;
	uint8_t			remoteClosed;
	xsMachine		*the;
	xsSlot			*onReadable;
	xsSlot			*onWritable;
	xsSlot			*onError;
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
static void xs_tcp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void doClose(xsMachine *the, xsSlot *instance);

static uint32_t tcpWritable(TCP tcp);
static void tcpFlush(TCP tcp, uint8_t force);
static void tcpFreeOutput(TCP tcp);
static void tcpIdle(modTimer timer, void *refcon, int refconSize);

static const xsHostHooks ICACHE_RODATA_ATTR xsTCPHooks = {
	xs_tcp_destructor,
	xs_tcp_mark,
	NULL
};

void xs_tcp_constructor(xsMachine *the)
{
	TCP tcp;
	TCP from = NULL;
	uint8_t create = xsmcArgc > 0;
	uint8_t connect = 0, triggerable = 0, triggered = 0, nodelay = 0, format = kIOFormatBuffer, ready = 0;
	xsSlot *onReadable, *onWritable, *onError;
	int port;
	struct tcp_pcb *skt = NULL;
	TCPBuffer buffers = NULL;
	TCPOutBuffer out = NULL, outTail = NULL;
	uint32_t unwritten = 0, unacked = 0;
	ip_addr_t address;

	xsmcVars(1);

	if (create) {
		CHECK_NETWORK_SAFE();

		builtinInitIO();

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
			from = xsmcGetHostDataValidate(xsVar(0), (void *)&xsTCPHooks);
			if (!from->skt)
				xsUnknownError("invalid from");

			skt = from->skt;
		}
		else {
			char addrStr[32];

			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			if (!ipaddr_aton(xsmcToStringBuffer(xsVar(0), addrStr, sizeof(addrStr)), &address))
				xsUnknownError("invalid address");

			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			port = builtinGetSignedInteger(the, &xsVar(0)); 
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
		if (!from)
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

	builtinInitializeTarget(the);

	tcp->skt = from ? NULL : skt;
	tcp->buffers = buffers;
	tcp->ready = ready;
	tcp->out = out;
	tcp->outTail = outTail;
	tcp->unwritten = unwritten;
	tcp->unacked = unacked;

	tcp->triggerable = triggerable;
	if (triggerable) {
		tcp->onReadable = onReadable;
		tcp->onWritable = onWritable;
		tcp->onError = onError;
	}

	if (from) {
		if (ERR_OK != tcp_handoff_checked_safe(&from->skt, &tcp->skt, tcp, tcpReceive, tcpSent, tcpError))
			xsUnknownError("invalid from");
		skt = tcp->skt;

		builtinCriticalSectionBegin();
		if (from->buffers) {
			TCPBuffer walker = from->buffers;
			while (walker->next)
				walker = walker->next;
			walker->next = tcp->buffers;
			tcp->buffers = from->buffers;
			from->buffers = NULL;
		}
		tcp->out = from->out;
		tcp->outTail = from->outTail;
		tcp->unwritten = from->unwritten;
		tcp->unacked = from->unacked;
		from->out = from->outTail = NULL;
		from->unwritten = from->unacked = 0;
		triggered |= from->triggered;
		from->triggered = 0;
		builtinCriticalSectionEnd();

		tcp->ready = from->ready;

		if ((triggerable & kTCPWritable) && tcp_sndbuf(skt))
			triggered |= kTCPWritable;
		if ((triggerable & kTCPReadable) && tcp->buffers)
			triggered |= kTCPReadable;
		if (tcp->unwritten)
			triggered |= kTCPOutput;

		xsmcGet(xsVar(0), xsArg(0), xsID_from);
		doClose(the, &xsVar(0));
	}
	else {
		tcp_arg(skt, tcp);
		tcp_err(skt, tcpError);
		tcp_recv(skt, tcpReceive);
		tcp_sent(skt, tcpSent);
	}

	if (nodelay) {
		if (2 == nodelay)
			tcp_nagle_disable(skt);
		else
			tcp_nagle_enable(skt);
	}

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

	if (tcp->timer) {
		modTimerRemove(tcp->timer);
		tcp->timer = NULL;
	}

	tcpFreeOutput(tcp);

	// Marshaled: clearing callbacks and closing run as one step on the tcpip
	// task, where tcp->skt is dereferenced — serialized with tcpError clearing
	// it when lwIP frees the pcb on a received reset, so a dispatch never runs
	// against a torn-down record and a freed pcb is never closed again
	tcp->triggerable &= ~kTCPWritable;
	tcp_teardown_safe(&tcp->skt);

	while (tcp->buffers) {
		TCPBuffer buffer = tcp->buffers;
		tcp->buffers = buffer->next;
		pbuf_free_safe(buffer->pb);
		c_free(buffer);
	}

	c_free(data);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void doClose(xsMachine *the, xsSlot *instance)
{
	TCP tcp = xsmcGetHostData(*instance);
	if (tcp && xsmcGetHostDataValidate(*instance, (void *)&xsTCPHooks)) {
		tcp->ready = false;

		if (tcp->timer) {
			modTimerRemove(tcp->timer);
			tcp->timer = NULL;
		}

		tcp_teardown_safe(&tcp->skt);
		tcp->triggerable = 0;

		xsmcSetHostData(*instance, NULL);
		xsForget(tcp->obj);
		xsmcSetHostDestructor(*instance, NULL);
		builtinCriticalSectionBegin();
		tcp->triggered = 0;
		builtinCriticalSectionEnd();
		tcpRelease(tcp);
	}
}

void xs_tcp_close(xsMachine *the)
{
	TCP tcp = xsmcGetHostData(xsThis);

	if (tcp && xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks)) {
		tcp->more = false;
		tcpFlush(tcp, true);
	}

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
				builtinCriticalSectionBegin();
				tcp->buffers = buffer->next;
				builtinCriticalSectionEnd();
				tcp_recved_checked_safe(&tcp->skt, &tcp->remoteClosed, buffer->pb->tot_len);
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
	uint8_t value, more = false;

	if (tcp->error || !tcp->skt)
		return;

	if (!tcp->ready)
		xsUnknownError("not ready");

	if ((xsmcArgc > 1) && xsmcTest(xsArg(1))) {
		if (xsmcGet(xsResult, xsArg(1), xsID_more))
			more = xsmcToBoolean(xsResult);

		if (xsmcGet(xsResult, xsArg(1), xsID_byteLength)) {
			int byteLength = xsmcToInteger(xsResult);
			tcp->writeRemaining = (byteLength > 0) ? (uint32_t)byteLength : 0;
		}
	}

	if (kIOFormatBuffer == tcp->format)
		xsmcGetBufferReadable(xsArg(0), &buffer, &needed);
	else {
		needed = 1;
		value = (uint8_t)xsmcToInteger(xsArg(0));
		buffer = &value;
	}

	if (needed > tcpWritable(tcp))
		xsUnknownError("would block");

	uint16_t size = tcp->skt->mss;
	if ((0 == size) || (size > MODDEF_TCP_OUTPUTBUFFER_MAX))
		size = MODDEF_TCP_OUTPUTBUFFER_MAX;
	if (tcp->writeRemaining && (tcp->writeRemaining < size))
		size = (uint16_t)tcp->writeRemaining;

	// only the last buffer can have room
	TCPOutBuffer fill = tcp->outTail;
	uint32_t remaining = needed;
	if (fill && (fill->bytes < fill->size)) {
		uint16_t space = fill->size - fill->bytes;
		remaining = (remaining > space) ? remaining - space : 0;
	}
	else
		fill = C_NULL;

	// allocate before copy
	TCPOutBuffer first = C_NULL, last = C_NULL;
	while (remaining) {
		TCPOutBuffer out = c_malloc(sizeof(TCPOutBufferRecord) + size);
		if (!out) {
			while (first) {
				TCPOutBuffer next = first->next;
				c_free(first);
				first = next;
			}
			xsUnknownError("no memory");
		}

		out->next = C_NULL;
		out->size = size;
		out->bytes = 0;

		if (last)
			last->next = out;
		else
			first = out;
		last = out;

		remaining = (remaining > size) ? remaining - size : 0;
	}

	if (first) {
		if (tcp->outTail)
			tcp->outTail->next = first;
		else
			tcp->out = first;
		tcp->outTail = last;
		if (!fill)
			fill = first;
	}

	uint8_t *src = buffer;
	remaining = needed;
	while (remaining) {
		uint16_t use = fill->size - fill->bytes;
		if (use > remaining)
			use = (uint16_t)remaining;

		c_memcpy(fill->bytes + fill->data, src, use);
		fill->bytes += use;
		src += use;
		remaining -= use;

		if (remaining)
			fill = fill->next;
	}

	tcp->unwritten += needed;
	tcp->more = more;
	if (!more)
		tcp->writeRemaining = 0;
	else if (tcp->writeRemaining)
		tcp->writeRemaining = (tcp->writeRemaining > needed) ? tcp->writeRemaining - needed : 0;

	tcpTrigger(tcp, kTCPOutput);

	xsmcSetInteger(xsResult, tcpWritable(tcp));
}

void xs_tcp_get_remoteAddress(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);

	struct tcp_pcb *skt = tcp->skt;
	if (skt) {
		xsResult = xsStringBuffer(NULL, 40);
		ipaddr_ntoa_r(&skt->remote_ip, xsmcToString(xsResult), 40);
	}
}

void xs_tcp_get_remotePort(xsMachine *the)
{
	TCP tcp = xsmcGetHostDataValidate(xsThis, (void *)&xsTCPHooks);

	struct tcp_pcb *skt = tcp->skt;
	if (skt)
		xsmcSetInteger(xsResult, skt->remote_port);
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

uint32_t tcpWritable(TCP tcp)
{
	struct tcp_pcb *skt = tcp->skt;
	if (!skt || tcp->error)
		return 0;

	uint32_t available = tcp_sndbuf(skt);
#if defined(__ets__) && !ESP32
	if (available > MODDEF_TCP_OUTPUTBUFFER_MAX)
		available = MODDEF_TCP_OUTPUTBUFFER_MAX;
#endif
	return (available > tcp->unwritten) ? available - tcp->unwritten : 0;
}

void tcpFlush(TCP tcp, uint8_t force)
{
	if (!tcp->skt || tcp->error)
		return;

	uint8_t output = false;
	while (tcp->out) {
		struct tcp_pcb *skt = tcp->skt;
		if (!skt || tcp->error)
			return;

		TCPOutBuffer out = tcp->out;
		if (!out->bytes)
			break;

		if ((out->bytes < out->size) && !force && tcp->unacked)
			break;

		if (out->bytes > tcp_sndbuf(skt))
			break;

		err_t err = tcp_write_checked_safe(&tcp->skt, out->data, out->bytes, (out->next || tcp->more) ? TCP_WRITE_FLAG_MORE : 0);
		if (ERR_OK != err) {
			if (ERR_MEM == err)
				break;

			xsMachine *the = tcp->the;
			xsLog("tcp write failed %d\n", err);
			if (tcp->timer) {
				modTimerRemove(tcp->timer);
				tcp->timer = NULL;
			}
			tcpTrigger(tcp, kTCPError);
			return;
		}

		tcp->unwritten -= out->bytes;
		modInstrumentationAdjust(NetworkBytesWritten, out->bytes);

		builtinCriticalSectionBegin();
		tcp->unacked += out->bytes;
		builtinCriticalSectionEnd();

		tcp->out = out->next;
		if (!tcp->out)
			tcp->outTail = NULL;
		c_free(out);

		output = true;
	}

	if (output)
		tcp_output_checked_safe(&tcp->skt);

	if (tcp->unwritten) {
		if (!tcp->timer)
			tcp->timer = modTimerAdd(MODDEF_TCP_OUTPUTBUFFER_IDLE, 0, tcpIdle, &tcp, sizeof(tcp));
		else if (output)
			modTimerReschedule(tcp->timer, MODDEF_TCP_OUTPUTBUFFER_IDLE, 0);
	}
	else if (tcp->timer) {
		modTimerRemove(tcp->timer);
		tcp->timer = NULL;
	}
}

void tcpIdle(modTimer timer, void *refcon, int refconSize)
{
	TCP tcp = *(TCP *)refcon;

	tcp->timer = NULL;
	tcpFlush(tcp, true);
}

void tcpFreeOutput(TCP tcp)
{
	while (tcp->out) {
		TCPOutBuffer out = tcp->out;
		tcp->out = out->next;
		c_free(out);
	}
	tcp->outTail = NULL;
	tcp->unwritten = 0;
}

void tcpHold(TCP tcp)
{
	builtinCriticalSectionBegin();
	tcp->useCount += 1;
	builtinCriticalSectionEnd();
}

void tcpRelease(TCP tcp)
{
	uint8_t unused;

	builtinCriticalSectionBegin();
	tcp->useCount -= 1;
	unused = 0 == tcp->useCount;
	builtinCriticalSectionEnd();
	if (unused)
		xs_tcp_destructor(tcp);
}

void tcpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	TCP tcp = refcon;
	builtinCriticalSectionBegin();
	uint8_t triggered = tcp->triggered;
	tcp->triggered = 0;
	builtinCriticalSectionEnd();

	if (triggered & kTCPError) {
		triggered &= ~(kTCPOutput | kTCPWritable);
		tcp->error = true;
		tcp->ready = false;
	}

	if (tcp->error)
		tcpFreeOutput(tcp);
	else if (triggered & kTCPOutput)
		tcpFlush(tcp, false);

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
				xsmcSetInteger(xsResult, tcpWritable(tcp));
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
	TCP tcp = arg;
	tcp->ready = !err;
	tcpTrigger(tcp, err ? kTCPError : kTCPWritable);
	return ERR_OK;
}

void tcpError(void *arg, err_t err)
{
	TCP tcp = arg;

	if (NULL == tcp)		// callbacks cleared during teardown
		return;

	tcp->skt = NULL;		// "pcb is already freed when this callback is called" - so do not touch it
	tcp->triggerable &= ~kTCPWritable;
	if (kTCPError & tcp->triggerable)
		tcpTrigger(tcp, kTCPError);
}

err_t tcpReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, err_t err)
{
	TCP tcp = arg;
	TCPBuffer buffer;

	if (NULL == tcp) {		// callbacks cleared during teardown
		if (pb)
			pbuf_free(pb);
		return ERR_OK;
	}

	if ((NULL == pb) || (ERR_OK != err)) {
		tcp_recv(pcb, NULL);
		tcp_sent(pcb, NULL);
		tcp->remoteClosed = true;
		tcp->triggerable &= ~kTCPWritable;
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

	// xs_tcp_read pops and frees head nodes on the XS task while this runs
	// on the lwIP task; guard the pointer surgery so the tail walk cannot
	// traverse a node being detached and freed
	builtinCriticalSectionBegin();
	if (tcp->buffers) {
		TCPBuffer walker;

		for (walker = tcp->buffers; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	else
		tcp->buffers = buffer;
	builtinCriticalSectionEnd();

	if (tcp->triggerable & kTCPReadable)
		tcpTrigger(tcp, kTCPReadable);

	return ERR_OK;
}

err_t tcpSent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	TCP tcp = arg;

	if (NULL == tcp)		// callbacks cleared during teardown
		return ERR_OK;

	builtinCriticalSectionBegin();
	tcp->unacked = (tcp->unacked > len) ? tcp->unacked - len : 0;
	builtinCriticalSectionEnd();

	// kTCPOutput releases a partial buffer held waiting for this acknowledgement
	tcpTrigger(tcp, kTCPOutput | kTCPWritable);
	return ERR_OK;
}

void tcpTrigger(TCP tcp, uint8_t trigger)
{
	builtinCriticalSectionBegin();
	uint8_t triggered = tcp->triggered;
	tcp->triggered |= trigger;
	builtinCriticalSectionEnd();

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
	TCPBuffer		buffers;
	uint8_t			closed;
};
typedef struct ListenerPendingRecord ListenerPendingRecord;
typedef struct ListenerPendingRecord *ListenerPending;

static void listenerPendingDiscard(ListenerPending pending);

struct ListenerRecord {
	struct tcp_pcb	*skt;
	xsSlot			obj;
	ListenerPending	pending;
	uint8_t			triggered;
	xsMachine		*the;
	xsSlot			*onReadable;
};
typedef struct ListenerRecord ListenerRecord;
typedef struct ListenerRecord *Listener;

static err_t listenerAccept(void *arg, struct tcp_pcb *skt, err_t err);
static void listenerPendingError(void *arg, err_t err);
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

	if (kIOFormatSocketTCP != builtinInitializeFormat(the, kIOFormatSocketTCP))
		xsRangeError("unimplemented");

	skt = tcp_new_safe();
	if (!skt)
		xsUnknownError("no socket");

	skt->so_options |= SOF_REUSEADDR;
	if (tcp_bind_safe(skt, &address, (uint16_t)port)) {
		tcp_close_safe(skt);
		xsUnknownError("socket bind");
	}

	struct tcp_pcb *lstn = tcp_listen_safe(skt);
	if (!lstn) {
		tcp_close_safe(skt);
		xsRangeError("no memory");
	}
	skt = lstn;

	listener = c_calloc(1, sizeof(ListenerRecord));
	if (!listener) {
		tcp_close_safe(skt);
		xsRangeError("no memory");
	}

	modInstrumentationAdjust(NetworkSockets, +1);

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
		tcp_teardown_safe(&pending->skt);
		listenerPendingDiscard(pending);
	}

	c_free(listener);

	modInstrumentationAdjust(NetworkSockets, -1);
}

static void listenerPendingDiscard(ListenerPending pending)
{
	while (pending->buffers) {
		TCPBuffer buffer = pending->buffers;
		pending->buffers = buffer->next;
		pbuf_free_safe(buffer->pb);
		c_free(buffer);
	}
	c_free(pending);
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

	while (true) {
		ListenerPending pending;

		builtinCriticalSectionBegin();
		pending = listener->pending;
		if (pending && (NULL == pending->skt))
			listener->pending = pending->next;
		else
			pending = NULL;
		builtinCriticalSectionEnd();

		if (NULL == pending)
			break;

		listenerPendingDiscard(pending);
	}

	if (NULL == listener->pending)
		return;

	xsResult = xsNewFunction0(xsArg(0));
	TCP tcp = xsmcGetHostDataValidate(xsResult, (void *)&xsTCPHooks);

	builtinCriticalSectionBegin();
	ListenerPending pending = listener->pending;
	listener->pending = pending->next;
	builtinCriticalSectionEnd();

	if (ERR_OK != tcp_handoff_checked_safe(&pending->skt, &tcp->skt, tcp, tcpReceive, tcpSent, tcpError)) {
		listenerPendingDiscard(pending);
		return;
	}

	tcp_accepted(tcp->skt);

	builtinCriticalSectionBegin();
	if (pending->buffers) {
		TCPBuffer walker = pending->buffers;
		while (walker->next)
			walker = walker->next;
		walker->next = tcp->buffers;
		tcp->buffers = pending->buffers;
		pending->buffers = NULL;
	}
	builtinCriticalSectionEnd();

	if ((tcp->triggerable & kTCPReadable) && tcp->buffers)
		tcpTrigger(tcp, kTCPReadable);
	if (pending->closed)
		tcpTrigger(tcp, kTCPError);

	c_free(pending);

	tcp->ready = true;
}

void xs_listener_get_port(xsMachine *the)
{
	Listener listener = xsmcGetHostDataValidate(xsThis, (void *)&xsListenerHooks);

	xsmcSetInteger(xsResult, listener->skt->local_port);
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

err_t listenerReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, err_t err)
{
	ListenerPending pending = arg;
	TCPBuffer buffer;

	if ((NULL == pb) || (ERR_OK != err)) {
		pending->closed = true;
		return ERR_OK;
	}

	buffer = c_malloc(sizeof(TCPBufferRecord));
	if (!buffer)
		return ERR_MEM;		// retry later

	buffer->next = NULL;
	buffer->pb = pb;
	buffer->bytes = pb->tot_len;
	buffer->fragment = pb;
	buffer->fragmentOffset = 0;

	modInstrumentationAdjust(NetworkBytesRead, buffer->bytes);

	builtinCriticalSectionBegin();
	if (pending->buffers) {
		TCPBuffer walker;

		for (walker = pending->buffers; walker->next; walker = walker->next)
			;
		walker->next = buffer;
	}
	else
		pending->buffers = buffer;
	builtinCriticalSectionEnd();

	return ERR_OK;
}

void listenerPendingError(void *arg, err_t err)
{
	ListenerPending pending = arg;
	pending->skt = NULL;		// "pcb is already freed when this callback is called"
}

err_t listenerAccept(void *arg, struct tcp_pcb *skt, err_t err)
{
	Listener listener = arg;
	ListenerPending pending, walker;

#ifdef mxDebug
	if (fxInNetworkDebugLoop(listener->the))
		return ERR_ABRT;		// lwip will close
#endif

	pending = c_calloc(1, sizeof(ListenerPendingRecord));
	if (!pending)
		return ERR_ABRT;		// lwip will close
	pending->skt = skt;

	tcp_arg(skt, pending);
	tcp_recv(skt, listenerReceive);
	tcp_err(skt, listenerPendingError);

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
