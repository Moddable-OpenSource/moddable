/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "xs.h"
#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"

#include "modInstrumentation.h"
#include "modTimer.h"

#define QAPI_NET_ENABLE_BSD_COMPATIBILITY
#include "qapi.h"
#include "qapi_ns_utils.h"
#include "qapi_netbuf.h"
#include "qapi_socket.h"
#include "qapi_netservices.h"
#include "qapi_ns_gen_v4.h"
#include "qapi_ns_gen_v6.h"

#include "qurt_error.h"
#include "qurt_thread.h"

#include "../socket/modSocket.h"

#ifndef SOMAXCONN
	#define SOMAXCONN 128
#endif

#define kTCP (0)
#define kUDP (1)
#define kRAW (2)

#define RESOLVER_THREAD_PRIORITY	19
#define RESOLVER_THREAD_STACK_SIZE	2048
#define LISTENER_THREAD_PRIORITY	19
#define LISTENER_THREAD_STACK_SIZE	2048

#define kRxBufferSize 1024
#define kTxBufferSize 1024

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

struct xsSocketRecord {
	xsSocket					next;
	
	xsSlot						obj;
	xsMachine					*the;

    int32_t						skt;
	int							port;
	char						host[256];
	
	uint8_t						useCount;
	uint8_t						connected;
	uint8_t						done;
	uint8_t						kind;		// kTCP or kUDP

	uint8_t						*readBuffer;
	int32_t						readBytes;

	int32_t						writeBytes;
	uint8_t						writeBuf[kTxBufferSize];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

struct xsListenerRecord {
	xsMachine			*the;
	xsSlot				obj;

	int32_t				skt;
	xsSocket			pending;
};

typedef struct {
	xsListener			xsl;
	int32_t				skt;
} xsListenerConnectionRecord, *xsListenerConnection;

static xsSocket gSockets = NULL;
static modTimer gTimer = NULL;

static int doFlushWrite(xsSocket xss);
static void doDestructor(xsSocket xss);

static void resolver_task(void *pvParameter);
static void listener_task(void *pvParameter);

static void socketTimerCallback(modTimer timer, void *refcon, int refconSize);

static void socketConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketDisconnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketWritten(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketError(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void listenerConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void socketUpUseCount(xsMachine *the, xsSocket xss);
static void socketDownUseCount(xsMachine *the, xsSocket xss);

void socketUpUseCount(xsMachine *the, xsSocket xss)
{
	xss->useCount += 1;
}

void socketDownUseCount(xsMachine *the, xsSocket xss)
{
	xss->useCount -= 1;
	if (xss->useCount <= 0) {
		xsDestructor destructor = xsGetHostDestructor(xss->obj);
		xsmcSetHostData(xss->obj, NULL);
		(*destructor)(xss);
	}
}

void xs_socket(xsMachine *the)
{
	xsSocket xss;
	qurt_thread_attr_t attr;
	qurt_thread_t thread;
	uint8_t result;	
	int ttl = 0;
	int port = 0;
	uint32_t multiaddr;
	struct sockaddr_in address;
			
	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_listener)) {
		xsListener xsl;
		xsmcGet(xsVar(0), xsArg(0), xsID_listener);
		xsl = xsmcGetHostData(xsVar(0));
		xss = xsl->pending;
		xsl->pending = NULL;

		xss->obj = xsThis;
		xsmcSetHostData(xsThis, xss);
		xsRemember(xss->obj);
		
		modInstrumentationAdjust(NetworkSockets, 1);
		
		if (!gSockets)
			gSockets = xss;
		else {
			xss->next = gSockets;
			gSockets = xss;
		}

		if (NULL == gTimer)
			gTimer = modTimerAdd(0, 20, socketTimerCallback, NULL, 0);
			
		return;
	}

	xss = c_calloc(sizeof(xsSocketRecord), 1);
	if (!xss)
		xsUnknownError("no memory");
	
	xsmcSetHostData(xsThis, xss);

	xss->obj = xsThis;
	xss->the = the;
	xss->useCount = 1;

	xss->kind = kTCP;
	if (xsmcHas(xsArg(0), xsID_kind)) {
		char *kind;

		xsmcGet(xsVar(0), xsArg(0), xsID_kind);
		kind = xsmcToString(xsVar(0));
		if (0 == c_strcmp(kind, "TCP"))
			;
		else if (0 == c_strcmp(kind, "UDP")) {
			xss->kind = kUDP;
			if (xsmcHas(xsArg(0), xsID_multicast)) {
				char temp[256];
				xsmcGet(xsVar(0), xsArg(0), xsID_multicast);
				xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
				if (0 != inet_pton(AF_INET, temp, &multiaddr))
					xsUnknownError("invalid multicast IP address");

				ttl = 1;
				if (xsmcHas(xsArg(0), xsID_ttl)) {
					xsmcGet(xsVar(0), xsArg(0), xsID_ttl);
					ttl = xsmcToInteger(xsVar(0));
				}
			}
		}
		else if (0 == c_strcmp(kind, "RAW"))
			xss->kind = kRAW;
		else
			xsUnknownError("invalid socket kind");
	}


	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
	}
	xss->port = port;
	
	if (kTCP == xss->kind)
		xss->skt = socket(AF_INET, SOCK_STREAM, 0);
	else if (kUDP == xss->kind)
		xss->skt = socket(AF_INET, SOCK_DGRAM, 0);
	else if (kRAW == xss->kind) {
		uint16_t protocol;
		xsmcGet(xsVar(0), xsArg(0), xsID_protocol);
		protocol = xsmcToInteger(xsVar(0));
		xss->skt = socket(AF_INET, SOCK_RAW, protocol);
	}

	if (-1 == xss->skt)
		xsUnknownError("create socket failed");

	c_memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if (-1 == bind(xss->skt, (struct sockaddr *)&address, sizeof(address)))
		xsUnknownError("bind socket failed");
		
	setsockopt(xss->skt, SOL_SOCKET, SO_NBIO, NULL, 0);

	modInstrumentationAdjust(NetworkSockets, 1);
	xsRemember(xss->obj);

	if (!gSockets)
		gSockets = xss;
	else {
		xss->next = gSockets;
		gSockets = xss;
	}

	if (NULL == gTimer)
		gTimer = modTimerAdd(0, 20, socketTimerCallback, NULL, 0);
		
	if (kUDP == xss->kind) {
		if (ttl) {
			uint32_t deviceId = qca4020_wlan_get_active_device();
			const char *devName;
			struct ip_mreq group;
	 		if (QAPI_OK == qapi_Net_Get_Wlan_Dev_Name(deviceId, &devName)) {
	 			uint32_t addr;
				if (QAPI_OK == qapi_Net_IPv4_Config(devName, QAPI_NET_IPV4CFG_QUERY_E, &addr, NULL, NULL)) {
					group.imr_interface = addr;
					group.imr_multiaddr = multiaddr;
					if (QAPI_OK != setsockopt(xss->skt, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&group, sizeof(group)))
						xsUnknownError("IP add membership failed");
					if (QAPI_OK != setsockopt(xss->skt, IPPROTO_IP, IP_TTL_OPT, (void*)&ttl, sizeof(ttl)))
						xsUnknownError("IP TTL failed");				
				}
			}
		}
		return;
	}
	if (kRAW == xss->kind) {
		return;
	}
	
	if (xsmcHas(xsArg(0), xsID_host)) {
		if (0 == qapi_Net_DNSc_Is_Started()) {
			qapi_Net_DNSc_Command(QAPI_NET_DNS_START_E);
			if (0 == qapi_Net_DNSc_Is_Started())
				xsUnknownError("dns initialization failed");
		}
		
		xsmcGet(xsVar(0), xsArg(0), xsID_host);
		xsmcToStringBuffer(xsVar(0), xss->host, sizeof(xss->host));
		
		qurt_thread_attr_init(&attr);
		qurt_thread_attr_set_name(&attr, "resolver");
		qurt_thread_attr_set_priority(&attr, RESOLVER_THREAD_PRIORITY);
		qurt_thread_attr_set_stack_size(&attr, RESOLVER_THREAD_STACK_SIZE);
		result = qurt_thread_create(&thread, &attr, resolver_task, xss);
		if (QURT_EOK != result)
			xsUnknownError("create resolver thread failed");
	}
	else
		xsUnknownError("host required in dictionary");
}

void resolver_task(void *pvParameter)
{
	xsSocket xss = (xsSocket)pvParameter;
	xsMachine *the = xss->the;
	uint8_t pass = 0;	// 0 = resolve host, 1 = connect to host

	while (pass < 2) {
		if (0 == pass) {
			struct qapi_hostent_s *host;
			host = gethostbyname(xss->host);
			if (host && (host->h_addrtype == AF_INET)) {
				char ip[20];
				struct in_addr addr;
				addr.s_addr = *(u_long *)host->h_addr_list[0];
				inet_ntop(AF_INET, &addr, ip, 20);
				if (c_strlen(ip) > 0) {
					struct sockaddr_in to = {0};
					to.sin_addr.s_addr = addr.s_addr;
					to.sin_port = htons(xss->port);
					to.sin_family = AF_INET;
					connect(xss->skt, (struct sockaddr*)&to, sizeof(struct sockaddr_in));
					++pass;
				}
				else {
					modMessagePostToMachine(xss->the, NULL, 0, socketError, xss);
					break;
				}
			}
			else {
				modMessagePostToMachine(xss->the, NULL, 0, socketError, xss);		
				break;
			}
		}
		else if (1 == pass) {
			if (!xss->done && !xss->connected) {
				int32_t result;
				qapi_fd_set_t rset;
				qapi_fd_zero(&rset);
				qapi_fd_set(xss->skt, &rset);
				result = qapi_select(&rset, NULL, NULL, 20);
				if (result < 0) {
					xss->done = true;	// socket no longer valid
					modMessagePostToMachine(the, NULL, 0, socketDisconnected, xss);
					++pass;
				}
				else {
					xss->connected = true;
					modMessagePostToMachine(the, NULL, 0, socketConnected, xss);
					++pass;
				}
			}
			else
				break;
		}
	}
		
	qurt_thread_stop();
}

void listener_task(void *pvParameter)
{
	xsListener xsl = (xsListener)pvParameter;
	uint8_t done = false;
	struct sockaddr_in addr;
	struct sockaddr *from = (struct sockaddr *)&addr;
	int32_t fromlen, result;
		
	while (!done) {
		modDelayMilliseconds(50);
		result = qapi_accept(xsl->skt, from, &fromlen);
		if (result > 0) {
			xsListenerConnectionRecord xslc;
			xslc.skt = result;
			xslc.xsl = xsl;
			modMessagePostToMachine(xsl->the, (uint8_t*)&xslc, sizeof(xslc), listenerConnected, 0);
		}
	}
				
	qurt_thread_stop();
}

void socketConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
		
	socketUpUseCount(the, xss);
	
	xsBeginHost(the);
	xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
	xsEndHost(the);
	
	socketDownUseCount(the, xss);
}
						
void socketDisconnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	
	if (xss->done)
		return;
		
	socketUpUseCount(the, xss);
	
	xsBeginHost(the);
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
	xsEndHost(the);
	
	socketDownUseCount(the, xss);
	
	doDestructor(xss);
}

void socketReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	uint8_t buffer[kRxBufferSize];
	int count;

	if (xss->done)
		return;
		
	socketUpUseCount(the, xss);
	
	count = recv(xss->skt, (char*)buffer, sizeof(buffer), 0);
	if (count < 0) {
		xsBeginHost(the);
			xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
		xsEndHost(the);
		socketDownUseCount(the, xss);
		doDestructor(xss);
		return;
	}
	else if (count > 0) {
		modInstrumentationAdjust(NetworkBytesRead, count);

		xss->readBuffer = buffer;
		xss->readBytes = count;

		xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count));
		xsEndHost(the);

		xss->readBuffer = NULL;
		xss->readBytes = 0;
	}
	
	socketDownUseCount(the, xss);
}

void socketWritten(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(sizeof(xss->writeBuf)));
	xsEndHost(the);
}

void socketError(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	//int32_t err = qapi_errno(xss->skt);	
	
	socketUpUseCount(the, xss);
	
	xsBeginHost(the);
	xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));		//@@ report the error value
	xsEndHost(the);

	socketDownUseCount(the, xss);
}

void listenerConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsListenerConnection xslc = (xsListenerConnection)message;
	xsListener xsl = xslc->xsl;
	xsSocket xss = c_calloc(sizeof(xsSocketRecord), 1);
	xss->the = xsl->the;
	xss->useCount = 1;
	xss->connected = 1;
	xss->skt = xslc->skt;
	xsl->pending = xss;

	xsBeginHost(xsl->the);
	xsCall1(xsl->obj, xsID_callback, xsInteger(kListenerMsgConnect));
	xsEndHost(xsl->the);

	xsl->pending = NULL;	
}

void socketTimerCallback(modTimer timer, void *refcon, int refconSize)
{
	xsSocket xss;
	xsSocket closed[10];
	uint8_t i, count = 0;
	
	// service readable sockets and collect disconnected sockets
	for (xss = gSockets; xss; xss = xss->next) {
		if (xss->connected || (kUDP == xss->kind)) {
			qapi_fd_set_t rset;
			int32_t result;
			qapi_fd_zero(&rset);
			qapi_fd_set(xss->skt, &rset);
			result = qapi_select(&rset, NULL, NULL, 0);
			if ((result < 0) && !xss->done) {
				xss->done = true;
				closed[count++] = xss;
			}
			else if (result > 0) {
				socketReadable(xss->the, xss, NULL, 0);
			}
		}
	}

	// close disconnected sockets
	for (i = 0; i < count; ++i) {
		xss = closed[i];
		socketDisconnected(xss->the, xss, NULL, 0);
	}
}

void doDestructor(xsSocket xss)
{
	xsSocket walker, prev = NULL;
	xss->done = 1;
	
	for (walker = gSockets; NULL != walker; prev = walker, walker = walker->next) {
		if (xss == walker) {
			if (NULL == prev)
				gSockets = walker->next;
			else
				prev->next = walker->next;
			break;
		}
	}	
	if (NULL == gSockets && NULL != gTimer) {
		modTimerRemove(gTimer);
		gTimer = NULL;
	}
		
	if (-1 != xss->skt) {
		modInstrumentationAdjust(NetworkSockets, -1);
		close(xss->skt);
		xss->skt = -1;
	}
}

void xs_socket_destructor(void *data)
{
	xsSocket xss = data;

	if (xss) {
		doDestructor(xss);
		c_free(xss);
	}
}

void xs_socket_close(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);

	if ((NULL == xss) || xss->done)
		xsUnknownError("close on closed socket");

	xsForget(xss->obj);

	xss->done = 1;
	socketDownUseCount(the, xss);
}

void xs_socket_get(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	const char *name = xsmcToString(xsArg(0));

	if (0 == c_strcmp(name, "REMOTE_IP")) {
		char *out;
		struct sockaddr_in addr;
		int32_t addrlen = sizeof(addr);

		xsResult = xsStringBuffer(NULL, 4 * 5);
		out = xsmcToString(xsResult);

		getpeername(xss->skt, (struct sockaddr *)&addr, &addrlen);
		inet_ntop(AF_INET, &addr, out, 4 * 5);
	}
}

void xs_socket_read(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	xsType dstType;
	int argc = xsmcArgc;
	uint16_t srcBytes;
	unsigned char *srcData;

	if ((NULL == xss) || xss->done)
		xsUnknownError("read on closed socket");

	if (!xss->readBuffer || !xss->readBytes) {
		if (0 == argc)
			xsResult = xsInteger(0);
		return;
	}

	srcData = xss->readBuffer;
	srcBytes = xss->readBytes;

	if (0 == argc) {
		xsResult = xsInteger(srcBytes);
		return;
	}

	// address limiter argument (count or terminator)
	if (argc > 1) {
		xsType limiterType = xsmcTypeOf(xsArg(1));
		if ((xsNumberType == limiterType) || (xsIntegerType == limiterType)) {
			uint16_t count = xsmcToInteger(xsArg(1));
			if (count < srcBytes)
				srcBytes = count;
		}
		else
			if (xsStringType == limiterType) {
				char *str = xsmcToString(xsArg(1));
				char terminator = c_read8(str);
				if (terminator) {
					unsigned char *t = (unsigned char *)c_strchr((char *)srcData, terminator);
					if (t) {
						uint16_t count = (t - srcData) + 1;		// terminator included in result
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
			xsResult = xsStringBuffer((char *)srcData, srcBytes);
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
					xsmcSetArrayBuffer(xsResult, srcData, srcBytes);
				else
					xsUnknownError("unsupported output type");
			}
		}
	}

	xss->readBuffer += srcBytes;
	xss->readBytes -= srcBytes;
}

void xs_socket_write(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	uint8_t *dst;
	uint16_t available, needed = 0;
	unsigned char pass, arg;

	if ((NULL == xss) || xss->done)
		xsUnknownError("write on closed socket");

	if (kUDP == xss->kind) {
		char temp[64];

		xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));

		int len = xsmcGetArrayBufferLength(xsArg(2));
		uint8_t *buf = xsmcToArrayBuffer(xsArg(2));

		struct sockaddr_in dest_addr = { 0 };

		int port = xsmcToInteger(xsArg(1));

		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(port);
		inet_pton(AF_INET, temp, &dest_addr.sin_addr.s_addr);
		
		int result = sendto(xss->skt, (char*)buf, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (result < 0)
			xsUnknownError("sendto failed");
			
		modInstrumentationAdjust(NetworkBytesWritten, len);
		
		return;
	}
	
	if (kRAW == xss->kind) {
		char temp[64];

		xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));

		int len = xsmcGetArrayBufferLength(xsArg(1));
		uint8_t *buf = xsmcToArrayBuffer(xsArg(1));

		struct sockaddr_in dest_addr = { 0 };
		dest_addr.sin_family = AF_INET;
		inet_pton(AF_INET, temp, &dest_addr.sin_addr.s_addr);
		
		int result = sendto(xss->skt, (char*)buf, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (result < 0)
			xsUnknownError("sendto failed");
			
		modInstrumentationAdjust(NetworkBytesWritten, len);
		return;
	}

	available = sizeof(xss->writeBuf) - xss->writeBytes;
	if (0 == argc) {
		xsResult = xsInteger(available);
		return;
	}

	dst = xss->writeBuf + xss->writeBytes;
	for (pass = 0; pass < 2; pass++) {
		for (arg = 0; arg < argc; arg++) {
			xsType t = xsmcTypeOf(xsArg(arg));

			if (xsStringType == t) {
				char *msg = xsmcToString(xsArg(arg));
				int msgLen = c_strlen(msg);
				if (0 == pass)
					needed += msgLen;
				else {
					c_memcpy(dst, msg, msgLen);
					dst += msgLen;
				}
			}
			else if ((xsNumberType == t) || (xsIntegerType == t)) {
				if (0 == pass)
					needed += 1;
				else
					*dst++ = (unsigned char)xsmcToInteger(xsArg(arg));
			}
			else if (xsReferenceType == t) {
				void *msg;
				xsUnsignedValue msgLen;

				xsmcGetBufferReadable(xsArg(arg), (void **)&msg, &msgLen);
				if (0 == pass)
					needed += msgLen;
				else {
					c_memcpy(dst, msg, msgLen);
					dst += msgLen;
				}
			}
			else
				xsUnknownError("unsupported type for write");
		}

		if ((0 == pass) && (needed > available))
			xsUnknownError("can't write all data");
	}

	xss->writeBytes = dst - xss->writeBuf;

	if (doFlushWrite(xss))
		xsUnknownError("write failed");
}

int doFlushWrite(xsSocket xss)
{
	int ret;

	ret = send(xss->skt, (char*)xss->writeBuf, xss->writeBytes, 0);
	if (ret < 0)
		return -1;

	modInstrumentationAdjust(NetworkBytesWritten, ret);

	if (ret > 0) {
		if (ret < xss->writeBytes) {
			xss->writeBytes -= ret;	
			return -1;
		}
		xss->writeBytes -= ret;
		modMessagePostToMachine(xss->the, NULL, 0, socketWritten, xss);
	}

	return 0;
}

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	qurt_thread_attr_t attr;
	qurt_thread_t thread;
	uint8_t result;	
	uint16_t port = 0;
	struct sockaddr_in address = { 0 };

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}

	xsl = c_calloc(sizeof(xsListenerRecord), 1);

	xsl->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (-1 == xsl->skt)
		xsUnknownError("create socket failed");

	xsl->the = the;
	xsl->obj = xsThis;
	xsmcSetHostData(xsThis, xsl);
	xsRemember(xsl->obj);

	int yes = 1;
	setsockopt(xsl->skt, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if (-1 == bind(xsl->skt, (struct sockaddr *)&address, sizeof(address)))
		xsUnknownError("bind socket failed");

	qurt_thread_attr_init(&attr);
	qurt_thread_attr_set_name(&attr, "listener");
	qurt_thread_attr_set_priority(&attr, LISTENER_THREAD_PRIORITY);
	qurt_thread_attr_set_stack_size(&attr, LISTENER_THREAD_STACK_SIZE);
	result = qurt_thread_create(&thread, &attr, listener_task, xsl);
	if (QURT_EOK != result)
		xsUnknownError("create listener thread failed");

	if (-1 == listen(xsl->skt, SOMAXCONN))
		xsUnknownError("listen failed");
}

void xs_socket_suspend(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	if (xsl) {
		if (-1 != xsl->skt)
			close(xsl->skt);
		c_free(xsl);
	}
}

void xs_listener_close(xsMachine *the)
{
	xsListener xsl = xsmcGetHostData(xsThis);

	if (NULL == xsl)
		xsUnknownError("close on closed listener");

	xsForget(xsl->obj);
	xsmcSetHostData(xsThis, NULL);
	xs_listener_destructor(xsl);
}

