/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "mc.xs.h"

#include "modInstrumentation.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <stdbool.h>
#include <linux/ip.h>
#include <linux/if_ether.h>

#include "../socket/modSocket.h"

#ifndef SOMAXCONN
	#define SOMAXCONN 128
#endif

#define kTCP (0)
#define kUDP (1)
#define kRAW (2)

#define kRxBufferSize 1024
#define kTxBufferSize 1024

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

struct xsSocketRecord {
	xsSocket					next;
	
	xsSlot						obj;
	xsMachine					*the;

    int32_t						skt;
	uint16_t					port;
	char						host[256];
	
	uint8_t						useCount;
	uint8_t						connected;
	uint8_t						done;
	uint8_t						kind;		// kTCP, kUDP, kRAW
	uint8_t						protocol;	// for raw socket
	
	uint8_t						*readBuffer;
	int32_t						readBytes;

	int32_t						writeBytes;
	int32_t						unreportedSent;		// bytes sent to the socket but not yet reported to object as sent
	uint8_t						writeBuf[kTxBufferSize];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

struct xsListenerRecord {
	xsMachine			*the;
	xsSlot				obj;

	int32_t				skt;
	xsSocket			pending;
	guint				timer;
};

typedef struct {
	xsListener			xsl;
	int32_t				skt;
} xsListenerConnectionRecord, *xsListenerConnection;

static xsSocket gSockets = NULL;
static guint gTimer = 0;

static int doFlushWrite(xsSocket xss);
static void doDestructor(xsSocket xss);

static gboolean socketServiceTimerCallback(gpointer data);
static void resolverCallback(GObject *source_object, GAsyncResult *result, gpointer user_data);

static void socketConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void socketWritable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void listenerConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void socketUpUseCount(xsMachine *the, xsSocket xss);
static void socketDownUseCount(xsMachine *the, xsSocket xss);

typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);

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
	int ttl = 0;
	uint16_t port = 0;
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

		if (0 == gTimer)
			gTimer = g_timeout_add(20, socketServiceTimerCallback, NULL);
			
		return;
	}

	xss = (xsSocket)c_calloc(sizeof(xsSocketRecord), 1);
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
		xss->skt = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	else if (kUDP == xss->kind)
		xss->skt = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	else if (kRAW == xss->kind) {
		uint16_t protocol;
		xsmcGet(xsVar(0), xsArg(0), xsID_protocol);
		protocol = xss->protocol = xsmcToInteger(xsVar(0));
		xss->skt = socket(AF_INET, SOCK_RAW | SOCK_NONBLOCK, protocol);
	}

	if (-1 == xss->skt)
		xsUnknownError("create socket failed");

	modInstrumentationAdjust(NetworkSockets, 1);
	xsRemember(xss->obj);

	if (!gSockets)
		gSockets = xss;
	else {
		xss->next = gSockets;
		gSockets = xss;
	}

	if (0 == gTimer)
		gTimer = g_timeout_add(20, socketServiceTimerCallback, NULL);

	if (kUDP == xss->kind) {
		if (ttl) {
			int result, flag = 1;
			uint8_t loop = 0;
							
			result = setsockopt(xss->skt, SOL_SOCKET, SO_BROADCAST, (const void *)&flag, sizeof(flag));
			if (result >= 0)
				result = setsockopt(xss->skt, SOL_SOCKET, SO_REUSEPORT, (const void *)&flag, sizeof(flag));
			if (result >= 0)
				result = setsockopt(xss->skt, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
			if (result >= 0)
				result = setsockopt(xss->skt, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
				
			// join group
			if (result >= 0) {
				struct ip_mreq imr;
				imr.imr_multiaddr.s_addr = multiaddr;
				if (~0 != *(int *)&imr.imr_multiaddr.s_addr) {
					imr.imr_interface.s_addr = htonl(INADDR_ANY);
					result = setsockopt(xss->skt, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr));
				}
			}
			
			// bind
			if (result >= 0) {
				address.sin_family = AF_INET;
				address.sin_port = htons(xss->port);
				address.sin_addr.s_addr = htonl(INADDR_ANY); // Want to receive multicasts AND unicasts on this socket
				result = bind(xss->skt, (struct sockaddr *) &address, sizeof(address));
				if (0 != result)
					result = -1;
			}
			
			if (result < 0)
				xsUnknownError("UDP multicast setup failed");
		}
		return;		
	}
	if (kRAW == xss->kind) {
		return;
	}
	
	if (xsmcHas(xsArg(0), xsID_host)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_host);
		xsmcToStringBuffer(xsVar(0), xss->host, sizeof(xss->host));
		
		GResolver *resolver = g_resolver_get_default();
		if (NULL == resolver)
			xsUnknownError("no resolver");
		g_resolver_lookup_by_name_async(resolver, xss->host, NULL, resolverCallback, xss);
	}
	else
		xsUnknownError("host required in dictionary");
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
						
void socketReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	uint8_t buffer[kRxBufferSize];
	struct sockaddr_in saddr;
	socklen_t saddr_len = sizeof(saddr);
	int count;

	if (xss->done)
		return;
		
	socketUpUseCount(the, xss);
	
	if (kTCP == xss->kind)
		count = recv(xss->skt, (char*)buffer, sizeof(buffer), 0);
	else {
		count = recvfrom(xss->skt, (char*)buffer, sizeof(buffer), 0, (struct sockaddr*)&saddr, &saddr_len);
		
		// @@ verify that the protocol matches what we configured
		// @@ verify that ethernet header protocol matches when protocol is ICMP
		// @@ an alternative approach would be to check the source ip address matches the host passed to the constructor
		if (count > 0 && kRAW == xss->kind) {
			struct ethhdr *eth = (struct ethhdr *)buffer;
			struct iphdr *ip_packet = (struct iphdr *)buffer;
			if ((xss->protocol != ip_packet->protocol) || (1 == ip_packet->protocol && 41725 != ntohs(eth->h_proto)))
				goto bail;
		}
	}

	if (count == 0) {
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
		if (kTCP == xss->kind)
			xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count));
		else {
			xsResult = xsStringBuffer((char*)inet_ntoa(saddr.sin_addr), 4 * 5);

			if (kUDP == xss->kind)
				xsCall4(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count), xsResult, xsInteger(ntohs(saddr.sin_port)));
			else
				xsCall3(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count), xsResult);		
		}
		xsEndHost(the);

		xss->readBuffer = NULL;
		xss->readBytes = 0;
	}
	
bail:
	socketDownUseCount(the, xss);
}

static void socketWritable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = (xsSocket)refcon;
	
	if (xss->done)
		return;
	
	socketUpUseCount(the, xss);
	xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(sizeof(xss->writeBuf)));
	xsEndHost(the);
	socketDownUseCount(the, xss);
}

void listenerConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsListenerConnection xslc = (xsListenerConnection)message;
	xsListener xsl = xslc->xsl;
	xsSocket xss = (xsSocket)c_calloc(sizeof(xsSocketRecord), 1);
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

gboolean socketServiceTimerCallback(gpointer data)
{
	#define kMaxSockets 10
	xsSocket xss;
	uint8_t i, max, count;
	int result;
	xsSocket xsss[kMaxSockets];
	fd_set rfds, wfds;
	struct timeval tv;

	FD_ZERO(&wfds);
	FD_ZERO(&rfds);
	for (xss = gSockets, count = 0, max = -1; xss; xss = xss->next) {
		if (xss->done)
			continue;
		xsss[count] = xss;
		FD_SET(xss->skt, (!xss->connected || xss->unreportedSent) ? &wfds : &rfds);
		if (xss->skt > max)
			max = xss->skt;
		if (++count == kMaxSockets)
			break;
	}
	if (0 != count) {
		tv.tv_sec = tv.tv_usec = 0;
		result = select(max + 1, &rfds, &wfds, NULL, &tv);
		if (result > 0) {
			for (i = 0; i < count; ++i) {
				xsSocket xss = xsss[i];
				if (FD_ISSET(xss->skt, &wfds)) {
					if (!xss->connected) {
						xss->connected = true;
						modMessagePostToMachine(xss->the, NULL, 0, socketConnected, xss);
					}
					else if (0 != xss->unreportedSent) {
						xss->unreportedSent = 0;
						modMessagePostToMachine(xss->the, NULL, 0, socketWritable, xss);
					}
				}
				if (FD_ISSET(xss->skt, &rfds)) {
					modMessagePostToMachine(xss->the, NULL, 0, socketReadable, xss);
				}
			}
		}
	}
	
	return G_SOURCE_CONTINUE;
}

void resolverCallback(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
	xsSocket xss = (xsSocket)user_data;
	GResolver *resolver = (GResolver*)source_object;
	char *ip = NULL;
	GList *addresses = NULL;
	GInetAddress *address = NULL;

	addresses = g_resolver_lookup_by_name_finish(resolver, result, NULL);
	while (NULL != addresses) {
		address = (GInetAddress*)addresses->data;
		if (NULL != address &&
		    g_inet_address_get_family(address) == G_SOCKET_FAMILY_IPV4) {
			ip = g_inet_address_to_string(address);
			break;
		}
		addresses = addresses->next;
	}

	if (NULL != ip) {
		struct sockaddr_in addr;
		int result;
		c_memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		c_memcpy(&(addr.sin_addr), g_inet_address_to_bytes(address), g_inet_address_get_native_size(address));
		addr.sin_port = htons(xss->port);
		result = connect(xss->skt, (struct sockaddr*)&addr, sizeof(addr));
		if (result >= 0) {
			xss->connected = true;
			modMessagePostToMachine(xss->the, NULL, 0, socketConnected, xss);
		}
		g_free(ip);
	}

	if (NULL != addresses)
		g_resolver_free_addresses(addresses);
	g_object_unref(resolver);
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
	
	if (NULL == gSockets && 0 != gTimer) {
		g_source_remove(gTimer);
		gTimer = 0;
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
		socklen_t addrlen = sizeof(addr);

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
			else if (xsUndefinedType == limiterType) {
			}
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
	if (ret < 0) {
		return -1;
	}

	modInstrumentationAdjust(NetworkBytesWritten, ret);

	if (ret > 0) {
		if (ret < xss->writeBytes) {
			xss->writeBytes -= ret;	
			return -1;
		}
		xss->writeBytes -= ret;
		xss->unreportedSent += ret;
	}

	return 0;
}

void xs_socket_suspend(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

gboolean listenerServiceTimerCallback(gpointer data)
{
	xsListener xsl = (xsListener)data;
	int result;
	
	result = accept(xsl->skt, NULL, NULL);
	if (result > 0) {
		xsListenerConnectionRecord xslc;
		xslc.skt = result;
		xslc.xsl = xsl;
		modMessagePostToMachine(xsl->the, (uint8_t*)&xslc, sizeof(xslc), listenerConnected, 0);
	}	
	return G_SOURCE_CONTINUE;
}

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	uint16_t port = 0;
	struct sockaddr_in address = { 0 };

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}

	xsl = (xsListener)c_calloc(sizeof(xsListenerRecord), 1);

	xsl->skt = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_IP);
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

	xsl->timer = g_timeout_add(50, listenerServiceTimerCallback, xsl);

	if (-1 == listen(xsl->skt, SOMAXCONN))
		xsUnknownError("listen failed");
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	if (xsl) {
		if (-1 != xsl->skt)
			close(xsl->skt);
		if (0 != xsl->timer)
			g_source_remove(xsl->timer);
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

// @@ Simple modMessage local implementation that essentially just implements delayed callbacks to the same thread

typedef struct modMessageRecord modMessageRecord;
typedef modMessageRecord *modMessage;

struct modMessageRecord {
	xsMachine           *the;
	modMessageDeliver   callback;
	void                *refcon;
	uint16_t            length;
	uint8_t				message[1];
};

static gboolean modMessageCallback(gpointer data)
{
	modMessage msg = (modMessage)data;
	if (msg->callback)
		(msg->callback)(msg->the, msg->refcon, msg->message, msg->length);

	c_free(msg);
	return G_SOURCE_REMOVE;
}

int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon)
{
	modMessage msg = c_calloc(1, sizeof(modMessageRecord) + messageLength);
	if (!msg) return -1;

	msg->the = the;
	msg->callback = callback;
	msg->refcon = refcon;

	if (message && messageLength)
		c_memmove(msg->message, message, messageLength);
	msg->length = messageLength;
	
	g_timeout_add(1, modMessageCallback, msg);

	return 0;
}


