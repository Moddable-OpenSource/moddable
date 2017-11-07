/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
#include "xsesp.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values

#include "lwip/tcp.h"
#include "lwip/udp.h"

#include "modSocket.h"

#if ESP32
	typedef int8_t int8;
	typedef uint8_t uint8;
	typedef int16_t int16;
	typedef uint16_t uint16;

	typedef ip_addr_t ip_addr;
#else
	#define IP_ADDR4 IP4_ADDR
#endif

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

#define kTCP (0)
#define kUDP (1)
#define kTCPListener (2)

#define kPendingConnect (1 << 0)
#define kPendingError (1 << 1)
#define kPendingDisconnect (1 << 2)
#define kPendingReceive (1 << 3)
#define kPendingSent (1 << 4)
#define kPendingAcceptListener (1 << 5)

struct xsSocketUDPRemoteRecord {
	uint16_t			port;
	ip_addr_t			address;
};
typedef struct xsSocketUDPRemoteRecord xsSocketUDPRemoteRecord;
typedef xsSocketUDPRemoteRecord *xsSocketUDPRemote;

#define kReadQueueLength (8)
struct xsSocketRecord {
	xsSocket			next;

	xsSlot				obj;
	struct tcp_pcb		*skt;

	int8				useCount;
	uint8				closed;
	uint8				kind;
	uint8				pending;

	// above here same as xsListenerRecord

	struct udp_pcb		*udp;

	struct pbuf			*reader[kReadQueueLength];

	uint32_t			outstandingSent;

	unsigned char		*buf;
	struct pbuf			*pb;
	uint16				bufpos;
	uint16				buflen;
	uint16				port;
	uint8				constructed;
	uint8				remoteCount;

	xsSocketUDPRemoteRecord
						remote[1];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

#define kListenerPendingSockets (4)
struct xsListenerRecord {
	xsListener			next;

	xsSlot				obj;
	struct tcp_pcb		*skt;

	int8				useCount;
	uint8				closed;
	uint8				kind;
	uint8				pending;

	// above here same as xsSocketRecord

	xsSocket			accept[kListenerPendingSockets];
};

static xsSocket gSockets;		// N.B. this list contains both sockets and listeners

#define kSocketCallbackExpire (24 * 60 * 60 * 1000)		// close to never
static modTimer gTimerPending;

void xs_socket_destructor(void *data);

static void socketSetPending(xsSocket xss, uint8_t pending);
static void socketClearPending(modTimer timer, void *refcon, uint32_t refconSize);

static void socketMsgConnect(xsSocket xss);
static void socketMsgDisconnect(xsSocket xss);
static void socketMsgError(xsSocket xss);
static void socketMsgDataReceived(xsSocket xss);
static void socketMsgDataSent(xsSocket xss);

static void didFindDNS(const char *name, ip_addr_t *ipaddr, void *arg);
static err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err);
static err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err);
static err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len);
static void didReceiveUDP(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

static uint8 parseAddress(char *address, uint8 *ip);

//@@ didError

static void forgetSocket(xsSocket xss)
{
	xsSocket walker, prev = NULL;

	for (walker = gSockets; NULL != walker; prev = walker, walker = walker->next) {
		if (walker != xss)
			continue;

		if (!prev)
			gSockets = walker->next;
		else
			prev->next = walker->next;

		if ((NULL == gSockets) && gTimerPending) {
			modTimerRemove(gTimerPending);
			gTimerPending = NULL;
		}
		return;
	}

	modLog("unknown socket");
}

#define socketUpUseCount(the, xss) (xss->useCount += 1)

static void socketDownUseCount(xsMachine *the, xsSocket xss)
{
	xss->useCount -= 1;
	if (xss->useCount <= 0) {
		xsDestructor destructor = xsGetHostDestructor(xss->obj);
		xsmcSetHostData(xss->obj, NULL);
		xsForget(xss->obj);
		(*destructor)(xss);
	}
}

void xs_socket(xsMachine *the)
{
	xsSocket xss;
	ip_addr_t ipaddr;
	int port = 0;
	err_t err;
	char temp[DNS_MAX_NAME_LENGTH];
	unsigned char ip[4];
	int len, i;
	unsigned char waiting = 0;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_listener)) {
		xsListener xsl;
		xsmcGet(xsVar(0), xsArg(0), xsID_listener);
		xsl = xsmcGetHostData(xsVar(0));

		for (i = 0; i < kListenerPendingSockets; i++) {
			if (xsl->accept[i]) {
				uint8 j;

				xss = xsl->accept[i];
				xsl->accept[i] = NULL;

				xss->obj = xsThis;
				xsmcSetHostData(xsThis, xss);
				xss->constructed = true;
				socketUpUseCount(the, xss);
				xsRemember(xss->obj);

				xss->next = gSockets;
				gSockets = xss;

				socketUpUseCount(gThe, xss);

				for (j = 0; j < kReadQueueLength; j++) {
					if (xss->reader[j])
						socketSetPending(xss, kPendingReceive);
				}

				socketDownUseCount(gThe, xss);

				return;
			}
		}
		xsUnknownError("no socket avaiable from listener");
	}

	// allocate socket
	xss = calloc(1, sizeof(xsSocketRecord) + (sizeof(xsSocketUDPRemoteRecord) * (kReadQueueLength - 1)));
	if (!xss)
		xsUnknownError("no memory for socket record");

	xss->obj = xsThis;
	xss->constructed = true;
	xss->useCount = 1;
	xsmcSetHostData(xsThis, xss);
	xsRemember(xss->obj);

	xss->next = gSockets;
	gSockets = xss;

	modInstrumentationAdjust(NetworkSockets, 1);

	// determine socket kind
	xss->kind = kTCP;
	if (xsmcHas(xsArg(0), xsID_kind)) {
		char *kind;

		xsmcGet(xsVar(0), xsArg(0), xsID_kind);
		kind = xsmcToString(xsVar(0));
		if (0 == espStrCmp(kind, "TCP"))
			;
		else if (0 == espStrCmp(kind, "UDP"))
			xss->kind = kUDP;
		else
			xsUnknownError("invalid socket kind");
	}

	// prepare inputs
	if (kTCP == xss->kind) {
		if (xsmcHas(xsArg(0), xsID_host)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_host);
			xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
			ip_addr_t resolved;
			if (ERR_OK == dns_gethostbyname(temp, &resolved, didFindDNS, xss)) {
#if LWIP_IPV4 && LWIP_IPV6
				ip[0] = ip4_addr1(&resolved.u_addr.ip4);
				ip[1] = ip4_addr2(&resolved.u_addr.ip4);
				ip[2] = ip4_addr3(&resolved.u_addr.ip4);
				ip[3] = ip4_addr4(&resolved.u_addr.ip4);
#else
				ip[0] = ip4_addr1(&resolved);
				ip[1] = ip4_addr2(&resolved);
				ip[2] = ip4_addr3(&resolved);
				ip[3] = ip4_addr4(&resolved);
#endif
			}
			else
				waiting = 1;
		}
		else
		if (xsmcHas(xsArg(0), xsID_address)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
			if (!parseAddress(temp, ip))
				xsUnknownError("invalid IP address");
		}
		else
			xsUnknownError("invalid dictionary");
	}

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
	}

	xss->port = port;
	if (kTCP == xss->kind)
		xss->skt = tcp_new();
	else
		xss->udp = udp_new();

	if (!xss->skt && !xss->udp)
		xsUnknownError("failed to allocate socket");

	if (kTCP == xss->kind)
		err = tcp_bind(xss->skt, IP_ADDR_ANY, 0);
	else
		err = udp_bind(xss->udp, IP_ADDR_ANY, xss->port);
	if (err)
		xsUnknownError("socket bind failed");

	if (kTCP == xss->kind) {
		tcp_arg(xss->skt, xss);
		tcp_recv(xss->skt, didReceive);
		tcp_sent(xss->skt, didSend);
	}
	else
		udp_recv(xss->udp, (udp_recv_fn)didReceiveUDP, xss);

	if (waiting || (kUDP == xss->kind))
		return;

	IP_ADDR4(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
	err = tcp_connect(xss->skt, &ipaddr, port, didConnect);
	if (err)
		xsUnknownError("socket connect faileds");
}

void xs_socket_destructor(void *data)
{
	xsSocket xss = data;
	unsigned char i;

	if (!xss) return;

	if (xss->skt) {
		tcp_recv(xss->skt, NULL);
		tcp_sent(xss->skt, NULL);
		tcp_abort(xss->skt);
	}

	if (xss->udp)
		udp_remove(xss->udp);

	for (i = 0; i < kReadQueueLength - 1; i++) {
		if (xss->reader[i])
			pbuf_free(xss->reader[i]);
	}

	forgetSocket(xss);

	free(xss);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_socket_close(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);

	if (NULL == xss)
		xsUnknownError("close on closed socket");

	if (!xss->closed) {
		xss->closed = 1;
		socketDownUseCount(the, xss);
	}
}

void xs_socket_read(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	xsType dstType;
	int argc = xsmcArgc;
	uint16 srcBytes;
	unsigned char *srcData;

	if (NULL == xss)
		xsUnknownError("read on closed socket");

	if (!xss->buf || (xss->bufpos >= xss->buflen)) {
		if (0 == argc)
			xsResult = xsInteger(0);
		return;
	}

	srcData = xss->bufpos + (unsigned char *)xss->buf;
	srcBytes = xss->buflen - xss->bufpos;

	if (0 == argc) {
		xsResult = xsInteger(srcBytes);
		return;
	}

	// address limiter argument (count or terminator)
	if (argc > 1) {
		xsType limiterType = xsmcTypeOf(xsArg(1));
		if ((xsNumberType == limiterType) || (xsIntegerType == limiterType)) {
			uint16 count = xsmcToInteger(xsArg(1));
			if (count < srcBytes)
				srcBytes = count;
		}
		else
		if (xsStringType == limiterType) {
			char *str = xsmcToString(xsArg(1));
			char terminator = espRead8(str);
			if (terminator) {
				unsigned char *t = strchr(srcData, terminator);
				if (t) {
					uint16 count = (t - srcData) + 1;		// terminator included in result
					if (count < srcBytes)
						srcBytes = count;
				}
			}
		}
		else if (xsUndefinedType == limiterType)
			;
	}

	// generate output
	dstType = xsmcTypeOf(xsArg(0));

	if (xsNullType == dstType)
		xsResult = xsInteger(srcBytes);
	else if (xsReferenceType == dstType) {
		xsSlot *s1, *s2;

		s1 = &xsArg(0);

		xsmcVars(1);
		xsmcGet(xsVar(0), xsGlobal, xsID_String);
		s2 = &xsVar(0);
		if (s1->data[2] == s2->data[2])		//@@
			xsResult = xsStringBuffer(srcData, srcBytes);
		else {
			xsmcGet(xsVar(0), xsGlobal, xsID_Number);
			s2 = &xsVar(0);
			if (s1->data[2] == s2->data[2]) {		//@@
				xsResult = xsInteger(*srcData);
				srcBytes = 1;
			}
			else {
				xsmcGet(xsVar(0), xsGlobal, xsID_ArrayBuffer);
				s2 = &xsVar(0);
				if (s1->data[2] == s2->data[2])		//@@
					xsResult = xsArrayBuffer(srcData, srcBytes);
				else
					xsUnknownError("unsupported output type");
			}
		}
	}

	xss->bufpos += srcBytes;

	if (xss->bufpos == xss->buflen) {
		if (xss->pb)
			pbuf_free(xss->pb);
		xss->pb = NULL;

		xss->bufpos = xss->buflen = 0;
		xss->buf = NULL;
	}
}

void xs_socket_write(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	char *msg;
	size_t msgLen;
	u16_t available, needed = 0;
	err_t err;
	unsigned char pass, arg;

	if ((NULL == xss) || !(xss->skt || xss->udp)) {
		if (0 == argc) {
			xsResult = xsInteger(0);
			return;
		}
		xsUnknownError("write on closed socket");
	}

	if (xss->udp) {
		char temp[64];
		uint8 ip[4];
		unsigned char *data;
		struct pbuf *p;
		uint16 port;
		ip_addr_t dst;

		xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));
		if (!parseAddress(temp, ip))
			xsUnknownError("invalid IP address");

		IP_ADDR4(&dst, ip[0], ip[1], ip[2], ip[3]);

		port = xsmcToInteger(xsArg(1));

		needed = xsGetArrayBufferLength(xsArg(2));
		data = xsmcToArrayBuffer(xsArg(2));

		p = pbuf_alloc(PBUF_TRANSPORT, needed, PBUF_RAM);
		memcpy(p->payload, data, needed);
		err = udp_sendto(xss->udp, p, &dst, port);
		pbuf_free(p);

		if (ERR_OK != err)
			xsUnknownError("UDP send failed");

		modInstrumentationAdjust(NetworkBytesWritten, needed);

		return;
	}

	available = tcp_sndbuf(xss->skt);
	if (0 == argc) {
		xsResult = xsInteger(available);
		return;
	}

	for (pass = 0; pass < 2; pass++ ) {
		for (arg = 0; arg < argc; arg++) {
			xsType t = xsmcTypeOf(xsArg(arg));

			if (xsStringType == t) {
				char *msg = xsmcToString(xsArg(arg));
				int msgLen = espStrLen(msg);
				if (0 == pass)
					needed += msgLen;
				else {
					// pull string through a temporary buffer, as it may be in ROM
					while (msgLen) {
						char buffer[128];
						int use = msgLen;
						if (use > sizeof(buffer))
							use = sizeof(buffer);

						espMemCpy(buffer, msg, use);
						do {
							err = tcp_write(xss->skt, buffer, use, TCP_WRITE_FLAG_COPY);
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err)
								xsUnknownError("write error - string");

//							xsTrace("out of memory on string write. try again\n");
							modDelayMilliseconds(25);
						} while (true);

						msg += use;
						msgLen -= use;
					}
				}
			}
			else if ((xsNumberType == t) || (xsIntegerType == t)) {
				if (0 == pass)
					needed += 1;
				else {
					unsigned char byte = (unsigned char)xsmcToInteger(xsArg(arg));
					do {
						err = tcp_write(xss->skt, &byte, 1, TCP_WRITE_FLAG_COPY);
						if (ERR_OK == err)
							break;

						if (ERR_MEM != err)
							xsUnknownError("write error - number");

//						xsTrace("out of memory on number write. try again\n");
						modDelayMilliseconds(25);
					} while (true);
				}
			}
			else if (xsReferenceType == t) {
				if (xsmcIsInstanceOf(xsArg(arg), xsArrayBufferPrototype)) {
					int msgLen = xsGetArrayBufferLength(xsArg(arg));
					if (0 == pass)
						needed += msgLen;
					else {
						char *msg = xsmcToArrayBuffer(xsArg(arg));

						do {
							err = tcp_write(xss->skt, msg, msgLen, TCP_WRITE_FLAG_COPY);		// this assumes data is in RAM
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err)
								xsUnknownError("write error - ArrayBuffer");

//							xsTrace("out of memory on ArrayBuffer write. try again\n");
							modDelayMilliseconds(25);
						} while (true);
					}
				}
				else if (xsmcIsInstanceOf(xsArg(arg), xsTypedArrayPrototype)) {
					int msgLen, byteOffset;

					xsmcGet(xsResult, xsArg(arg), xsID_byteLength);
					msgLen = xsmcToInteger(xsResult);
					if (0 == pass)
						needed += msgLen;
					else {
						xsSlot tmp;
						char *msg;

						xsmcGet(tmp, xsArg(arg), xsID_byteOffset);
						byteOffset = xsmcToInteger(tmp);

						xsmcGet(tmp, xsArg(arg), xsID_buffer);
						msg = byteOffset + (char *)xsmcToArrayBuffer(tmp);

						do {
							err = tcp_write(xss->skt, msg, msgLen, TCP_WRITE_FLAG_COPY);		// this assumes data is in RAM
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err)
								xsUnknownError("write error - TypedArray");

//							xsTrace("out of memory on TypedArray write. try again\n");
							modDelayMilliseconds(25);
						} while (true);
					}
				}
				else
					xsUnknownError("unsupported type for write");
			}
			else
				xsUnknownError("unsupported type for write");
		}

		if ((0 == pass) && (needed > available))
			xsUnknownError("can't write all data");
	}

	xss->outstandingSent += needed;

	err = tcp_output(xss->skt);
	if (err)
		xsUnknownError("tcp_output error");
}

void socketMsgConnect(xsSocket xss)
{
	xsMachine *the = gThe;

	xsBeginHost(the);
	if (xss->skt)
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
	xsEndHost(the);
}

void socketMsgDisconnect(xsSocket xss)
{
	xsMachine *the = gThe;

	xsBeginHost(the);
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
	xsEndHost(the);
}

void socketMsgError(xsSocket xss)
{
	xsMachine *the = gThe;

	xsBeginHost(the);
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));		//@@ report the error value
	xsEndHost(the);
}

void socketMsgDataReceived(xsSocket xss)
{
	xsMachine *the = gThe;
	unsigned char i, readerCount;
	uint16_t tot_len;
	struct pbuf *pb, *walker;
	uint8_t one = 0;

	modCriticalSectionBegin();
	for (readerCount = 0; xss->reader[readerCount] && (readerCount < kReadQueueLength); readerCount++)
		;
	modCriticalSectionEnd();

	while (xss->reader[0] && readerCount--) {
		xsBeginHost(the);

		pb = xss->reader[0];

		modCriticalSectionBegin();
		for (i = 0; i < kReadQueueLength - 1; i++)
			xss->reader[i] = xss->reader[i + 1];
		xss->reader[kReadQueueLength - 1] = NULL;
		modCriticalSectionEnd();

		tot_len = pb->tot_len;

		if (NULL == pb->next) {
			one = 1;
			xss->pb = pb;
		}

		for (walker = pb; walker; walker = walker->next) {
			xss->buf = walker->payload;
			xss->bufpos = 0;
			xss->buflen = walker->len;

			xsTry {
				if (kTCP == xss->kind)
					xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(xss->buflen));
				else {
					ip_addr_t address = xss->remote[0].address;
					char *out;

					xsResult = xsStringBuffer(NULL, 4 * 5);
					out = xsmcToString(xsResult);
#if LWIP_IPV4 && LWIP_IPV6
					itoa(ip4_addr1(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr2(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr3(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr4(&address.u_addr.ip4), out, 10); out += strlen(out); *out = 0;
#else
					itoa(ip4_addr1(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr2(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr3(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr4(&address), out, 10); out += strlen(out); *out = 0;
#endif
					xsCall4(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(xss->buflen), xsResult, xsInteger(xss->remote[0].port));

					xss->remoteCount -= 1;
					c_memmove(&xss->remote[0], &xss->remote[1], xss->remoteCount * sizeof(xsSocketUDPRemoteRecord));
				}
			}
			xsCatch {
			}

			if (one)
				break;
		}

		if (xss->skt)
			tcp_recved(xss->skt, tot_len);

		xsEndHost(the);

		if (one) {
			if (xss->pb)
				pbuf_free(xss->pb);
		}
		else
			pbuf_free(pb);

		xss->buf = NULL;
	}
}

void socketMsgDataSent(xsSocket xss)
{
	xsMachine *the = gThe;

	xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(xss->skt ? tcp_sndbuf(xss->skt) : 0));
	xsEndHost(the);
}

void didFindDNS(const char *name, ip_addr_t *ipaddr, void *arg)
{
	xsSocket xss = arg;

	if (ipaddr)
		tcp_connect(xss->skt, ipaddr, xss->port, didConnect);
	else
		socketSetPending(xss, kPendingError);
}

err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err)
{
	xsSocket xss = arg;

	socketSetPending(xss, kPendingConnect);

	return ERR_OK;
}

err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	xsSocket xss = arg;
	unsigned char i;
	struct pbuf *walker;
	uint16 offset;

	if (!p) {
		tcp_recv(xss->skt, NULL);
		tcp_sent(xss->skt, NULL);
		if (ERR_OK == err)
			tcp_abort(xss->skt);		//@@ matches tcp_recv_null in tcp.c
		else
			modLog("...error so don't toss socket... just zero it out");
		xss->skt = 0;

		socketSetPending(xss, kPendingDisconnect);
		return ERR_OK;
	}

	modCriticalSectionBegin();
	for (i = 0; i < kReadQueueLength; i++) {
		if (NULL == xss->reader[i])
			break;
	}

	if (kReadQueueLength == i) {
		modCriticalSectionEnd();
		modLog("tcp read overflow!");
		pbuf_free(p);
		return ERR_MEM;
	}

	modInstrumentationAdjust(NetworkBytesRead, p->tot_len);

	xss->reader[i] = p;
	modCriticalSectionEnd();

	if (xss->constructed)
		socketSetPending(xss, err ? kPendingError : kPendingReceive);

	return ERR_OK;
}

err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	xsSocket xss = arg;

	modInstrumentationAdjust(NetworkBytesWritten, len);

	xss->outstandingSent -= len;
	if (0 == xss->outstandingSent)
		socketSetPending(xss, kPendingSent);

	return ERR_OK;
}

void didReceiveUDP(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *remoteAddr, u16_t remotePort)
{
	xsSocket xss = arg;
	unsigned char i;

	modCriticalSectionBegin();

	for (i = 0; i < kReadQueueLength; i++) {
		if (NULL == xss->reader[i])
			break;
	}

	if (kReadQueueLength == i) {
		modCriticalSectionEnd();
		modLog("udp read overflow!");
		pbuf_free(p);
		return;
	}

	xss->remote[xss->remoteCount].port = remotePort;
	xss->remote[xss->remoteCount].address = *remoteAddr;
	xss->remoteCount += 1;

	modInstrumentationAdjust(NetworkBytesRead, p->tot_len);
	xss->reader[i] = p;
	modCriticalSectionEnd();

	socketSetPending(xss, kPendingReceive);
}

static uint8 parseAddress(char *address, uint8_t *ip)
{
	char *p = address;
	int i;
	for (i = 0; i < 3; i++) {
		char *separator = strchr(p, (i < 3) ? '.' : 0);
		if (!separator)
			return 0;
		*separator = 0;
		ip[i] = (unsigned char)atoi(p);
		p = separator + 1;
	}
	ip[3] = (unsigned char)atoi(p);

	return 1;
}

static err_t didAccept(void * arg, struct tcp_pcb * newpcb, err_t err);

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	uint16 port = 0;
	err_t err;

	// allocate listener
	xsl = calloc(1, sizeof(xsListenerRecord));
	if (!xsl)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, xsl);

	modInstrumentationAdjust(NetworkSockets, 1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16)xsmcToInteger(xsVar(0));
	}

	xsl->obj = xsThis;
	socketUpUseCount(the, xsl);
	xsRemember(xsl->obj);

	xsl->kind = kTCPListener;

	xsl->skt = tcp_new();
	if (!xsl->skt)
		xsUnknownError("socket allocation failed");

	xsl->next = (xsListener)gSockets;
	gSockets = (xsSocket)xsl;

	err = tcp_bind(xsl->skt, IP_ADDR_ANY, port);
	if (err)
		xsUnknownError("socket bind");

	xsl->skt = tcp_listen(xsl->skt);

	tcp_arg(xsl->skt, xsl);

	tcp_accept(xsl->skt, didAccept);
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	uint8 i;

	if (!xsl) return;

	if (xsl->skt) {
		tcp_accept(xsl->skt, NULL);
		tcp_close(xsl->skt);
	}

	for (i = 0; i < kListenerPendingSockets; i++)
		xs_socket_destructor(xsl->accept[i]);

	forgetSocket((xsSocket)xsl);

	free(xsl);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_listener_close(xsMachine *the)
{
	xsListener xsl = xsmcGetHostData(xsThis);

	if ((NULL == xsl) || xsl->closed)
		xsUnknownError("close on closed listener");

	xsl->closed = 1;
	socketDownUseCount(the, (xsSocket)xsl);
}

static void listenerMsgNew(xsListener xsl)
{
	xsMachine *the = gThe;
	uint8 i;

	// service all incoming sockets currently in the list
	for (i = 0; i < kListenerPendingSockets; i++) {
		if (NULL == xsl->accept[i])
			continue;

		xsBeginHost(the);
		xsCall1(xsl->obj, xsID_callback, xsInteger(kListenerMsgConnect));
		xsEndHost(the);
	}
}

err_t didAccept(void * arg, struct tcp_pcb * newpcb, err_t err)
{
	xsListener xsl = arg;
	xsSocket xss;
	uint8 i;

	for (i = 0; i < kListenerPendingSockets; i++) {
		if (NULL == xsl->accept[i])
			break;
	}
	if (kListenerPendingSockets == i) {
		modLog("tcp accept queue full");
		tcp_abort(newpcb);
		return ERR_MEM;
	}

	xss = calloc(1, sizeof(xsSocketRecord) - sizeof(xsSocketUDPRemoteRecord));
	if (!xss) return ERR_MEM;

	xss->skt = newpcb;
	tcp_arg(xss->skt, xss);
	tcp_recv(xss->skt, didReceive);
	tcp_sent(xss->skt, didSend);

	xsl->accept[i] = xss;

	xss->kind = kTCP;

	socketUpUseCount(gThe, xsl);
	tcp_accepted(xsl->skt);

	socketSetPending((xsSocket)xsl, kPendingAcceptListener);

	modInstrumentationAdjust(NetworkSockets, 1);

	return ERR_OK;
}

void socketSetPending(xsSocket xss, uint8_t pending)
{
	modCriticalSectionBegin();

	if ((xss->pending & pending) == pending) {
		modCriticalSectionEnd();
		return;
	}

	if (!xss->pending) {
		socketUpUseCount(gThe, xss);

		if (gTimerPending)
			modTimerReschedule(gTimerPending, 0, kSocketCallbackExpire);
		else
			gTimerPending = modTimerAdd(0, kSocketCallbackExpire, socketClearPending, NULL, 0);
	}

	xss->pending |= pending;

	modCriticalSectionEnd();
}

void socketClearPending(modTimer timer, void *refcon, uint32_t refconSize)
{
	xsSocket walker = gSockets;

	while (walker) {
		uint8_t pending;
		xsSocket next;

		modCriticalSectionBegin();

		pending = walker->pending;
		walker->pending = 0;

		modCriticalSectionEnd();

		if (!pending) {
			walker = walker->next;
			continue;
		}

		if (pending & kPendingReceive)
			socketMsgDataReceived(walker);

		if (pending & kPendingSent)
			socketMsgDataSent(walker);

		if (pending & kPendingConnect)
			socketMsgConnect(walker);

		if (pending & kPendingDisconnect)
			socketMsgDisconnect(walker);

		if (pending & kPendingError)
			socketMsgError(walker);

		if (pending & kPendingAcceptListener)
			listenerMsgNew((xsListener)walker);

		next = walker->next;
		socketDownUseCount(gThe, walker);
		walker = next;
	}
}



