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

#include "xsPlatform.h"
#include "xsmc.h"
#include "modInstrumentation.h"

#include "mc.xs.h"			// for xsID_ values

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFSocket.h>
#include <CFNetwork/CFNetwork.h>
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

#include "../socket/modSocket.h"

#define kTCP (0)
#define kUDP (1)

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

struct xsSocketRecord {
	xsSlot						obj;
	xsMachine					*the;

    int							skt;
	int							port;

	CFSocketRef					cfSkt;
	CFRunLoopSourceRef			cfRunLoopSource;

	uint8_t						useCount;
	uint8_t						done;
	uint8_t						kind;		// kTCP or kUDP

	uint8_t						*readBuffer;
	int32_t						readBytes;

	int32_t						writeBytes;
	int32_t						unreportedSent;		// bytes sent to the socket but not yet reported to object as sent

	uint8_t						writeBuf[1024];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

#define kListenerPendingSockets (4)
struct xsListenerRecord {
	xsListener			next;

	xsMachine			*the;
	xsSlot				obj;

	CFSocketRef			cfSkt;
	CFRunLoopSourceRef	cfRunLoopSource;

	xsSocket			pending;
};

static void socketCallback(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef addr, const void *data, void *info);
static int doFlushWrite(xsSocket xss);
static void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info);

static void socketDownUseCount(xsMachine *the, xsSocket xss)
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
    int set;

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

		xss->cfRunLoopSource = CFSocketCreateRunLoopSource(NULL, xss->cfSkt, 0);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), xss->cfRunLoopSource, kCFRunLoopCommonModes);

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
		else if (0 == c_strcmp(kind, "UDP"))
			xss->kind = kUDP;
		else
			xsUnknownError("invalid socket kind");
	}

	CFSocketContext socketCtxt = {0, xss, NULL, NULL, NULL};
	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		xss->port = xsmcToInteger(xsVar(0));
	}
	else
		xsUnknownError("port required in dictionary");
		
	if (kTCP == xss->kind) 
		xss->cfSkt = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketConnectCallBack | kCFSocketReadCallBack | kCFSocketWriteCallBack, socketCallback, &socketCtxt);
	else
		xss->cfSkt = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_DGRAM, IPPROTO_UDP, kCFSocketConnectCallBack | kCFSocketReadCallBack | kCFSocketWriteCallBack, socketCallback, &socketCtxt);

	xss->skt = CFSocketGetNative(xss->cfSkt);
	if (xss->skt < 0)
		xsUnknownError("create socket failed");

	modInstrumentationAdjust(NetworkSockets, 1);
	xsRemember(xss->obj);

	set = 1;
	setsockopt(xss->skt, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

	fcntl(xss->skt, F_SETFL, O_NONBLOCK | fcntl(xss->skt, F_GETFL, 0));

	if ((kUDP == xss->kind) && xsmcHas(xsArg(0), xsID_multicast)) {
		struct ip_mreq imr;
		struct sockaddr_in address;
		char ip[64];
		int ttl = 1;
		int flag = 1;

		xsmcGet(xsVar(0), xsArg(0), xsID_multicast);
		xsmcToStringBuffer(xsVar(0), ip, sizeof(ip));

		if (xsmcHas(xsArg(0), xsID_ttl)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_ttl);
			ttl = xsmcToInteger(xsVar(0));
		}

		if (setsockopt(xss->skt, SOL_SOCKET, SO_BROADCAST, (const void *)&flag, sizeof(flag)) < 0)
			return;		//@@

		if (setsockopt(xss->skt, SOL_SOCKET, SO_REUSEPORT, (const void *)&flag, sizeof(flag)) < 0)
			return;		//@@

		u_char loop = 0;
		setsockopt(xss->skt, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

		if (setsockopt(xss->skt, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
			return;

		// join group
		imr.imr_multiaddr.s_addr = inet_addr(ip);
		if (~0 != *(int *)&imr.imr_multiaddr.s_addr) {
			imr.imr_interface.s_addr = htonl(INADDR_ANY);
			int foo = setsockopt(xss->skt, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr));
			if (foo == -1) {
				foo = errno;
				return;		//@@
			}
		}


		// bind
		address.sin_family      = AF_INET;
		address.sin_port        = htons(xss->port);		//@@
		address.sin_addr.s_addr = htonl(INADDR_ANY); // Want to receive multicasts AND unicasts on this socket
		if (0 != bind(xss->skt, (struct sockaddr *) &address, sizeof(address)))
			return;		//@@
	}

	xss->cfRunLoopSource = CFSocketCreateRunLoopSource(NULL, xss->cfSkt, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), xss->cfRunLoopSource, kCFRunLoopCommonModes);

	if (kUDP == xss->kind)
		return;

	CFHostRef cfHost;

	if (xsmcHas(xsArg(0), xsID_host)) {
		char *str;
		CFStringRef host;

		xsmcGet(xsVar(0), xsArg(0), xsID_host);
		str = xsmcToString(xsVar(0));
		host = CFStringCreateWithCString(NULL, (const char *)str, kCFStringEncodingUTF8);
		cfHost = CFHostCreateWithName(kCFAllocatorDefault, host);
		CFRelease(host);
	}
	else if (xsmcHas(xsArg(0), xsID_address)) {
		char *str;
		CFStringRef host;

		xsmcGet(xsVar(0), xsArg(0), xsID_address);
		str = xsmcToString(xsVar(0));
		host = CFStringCreateWithCString(NULL, (const char *)str, kCFStringEncodingUTF8);
		cfHost = CFHostCreateWithName(kCFAllocatorDefault, host);
		CFRelease(host);
	}
	else
		xsUnknownError("host required in dictionary");

	CFHostClientContext context = {0};
	context.info = xss;

	CFHostSetClient(cfHost, resolved, &context);
	CFHostScheduleWithRunLoop(cfHost, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	// resolve
	CFStreamError streamErr;
	if (!CFHostStartInfoResolution(cfHost, kCFHostAddresses, &streamErr))
		xsUnknownError("cannot resolve host address");
}

void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info)
{
	xsSocket xss = info;

	CFArrayRef cfArray = CFHostGetAddressing(cfHost, NULL);
	if (NULL == cfArray) {	// failed to resolve
		xsBeginHost(xss->the);
			xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));
		xsEndHost(xss->the);
		return;
	}
	NSData *address = CFArrayGetValueAtIndex(cfArray, CFArrayGetCount(cfArray) - 1);

	const UInt8 *bytes = CFDataGetBytePtr((__bridge CFDataRef)address);
//@@	CFIndex length = CFDataGetLength((__bridge CFDataRef)address);
//@@	if (length != sizeof(struct sockaddr_in))
//@@		xsUnknownError("unexpected sockaddr");

	struct sockaddr_in addr = *(struct sockaddr_in *)bytes;
	addr.sin_port = htons(xss->port);

	CFSocketError err = CFSocketConnectToAddress(xss->cfSkt, CFDataCreate(kCFAllocatorDefault, (const UInt8*)&addr, sizeof(addr)), (CFTimeInterval)10000);
	if (err) {
		xsBeginHost(xss->the);
			xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));
		xsEndHost(xss->the);
	}
}

static void doDestructor(xsSocket xss)
{
	xss->done = 1;

	if (xss->cfRunLoopSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), xss->cfRunLoopSource, kCFRunLoopCommonModes);
		CFRelease(xss->cfRunLoopSource);
		xss->cfRunLoopSource = NULL;
	}

	if (xss->cfSkt) {
		CFSocketInvalidate(xss->cfSkt);
		CFRelease(xss->cfSkt);
		xss->cfSkt = NULL;
		xss->skt = -1;

		modInstrumentationAdjust(NetworkSockets, -1);
	}

	if (xss->skt >= 0) {
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
}

void xs_socket_read(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	xsType dstType;
	int argc = xsmcArgc;
	int32_t srcBytes;
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
				unsigned char *t = (unsigned char *)memchr((char *)srcData, terminator, srcBytes);
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

		struct sockaddr_in dest_addr = {0};

		int port = xsmcToInteger(xsArg(1));

		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(port);
		dest_addr.sin_addr.s_addr = inet_addr(temp);

		int result = sendto(xss->skt, buf, len, 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
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
	for (pass = 0; pass < 2; pass++ ) {
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
				if (xsmcIsInstanceOf(xsArg(arg), xsArrayBufferPrototype)) {
					int msgLen = xsmcGetArrayBufferLength(xsArg(arg));
					if (0 == pass)
						needed += msgLen;
					else {
						char *msg = xsmcToArrayBuffer(xsArg(arg));
						c_memcpy(dst, msg, msgLen);
						dst += msgLen;
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
						msg = byteOffset + xsmcToArrayBuffer(tmp);
						c_memcpy(dst, msg, msgLen);
						dst += msgLen;
					}
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

void xs_socket_suspend(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void socketCallback(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef addr, const void *data, void *info)
{
	xsSocket xss = info;
	xsMachine *the = xss->the;

	if (xss->done)
		return;

	xss->useCount += 1;

	if (cbType & kCFSocketReadCallBack) {
		unsigned char buffer[2048];
		int count;

		if (kTCP == xss->kind) {
			count = read(xss->skt, buffer, sizeof(buffer));

			if (count <= 0) {
				xsBeginHost(the);
					xsCall1(xss->obj, xsID_callback, (count < 0) ? xsInteger(kSocketMsgError) : xsInteger(kSocketMsgDisconnect));
				xsEndHost(the);

				socketDownUseCount(the, xss);
				doDestructor(xss);
				return;
			}

			modInstrumentationAdjust(NetworkBytesRead, count);

			xss->readBuffer = buffer;
			xss->readBytes = count;

			xsBeginHost(the);
				xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count));
			xsEndHost(the);

			xss->readBuffer = NULL;
			xss->readBytes = 0;
		}
		else {
			struct sockaddr_in srcAddr;
			socklen_t srcAddrLen = sizeof(srcAddr);
			char srcAddrStr[INET_ADDRSTRLEN];

			count = recvfrom(xss->skt, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&srcAddr, &srcAddrLen);

			if (count <= 0)
				return;

			modInstrumentationAdjust(NetworkBytesRead, count);

			xss->readBuffer = buffer;
			xss->readBytes = count;

			inet_ntop(srcAddr.sin_family, &srcAddr.sin_addr, srcAddrStr, sizeof(srcAddrStr));

			xsBeginHost(the);
				xsmcSetString(xsResult, srcAddrStr);
				xsCall4(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(count), xsResult, xsInteger(ntohs(srcAddr.sin_port)));		//@@ port
			xsEndHost(the);

			xss->readBuffer = NULL;
			xss->readBytes = 0;
		}

	}

	if (cbType & kCFSocketConnectCallBack) {
		xsBeginHost(the);
			xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
		xsEndHost(the);
	}

	if (cbType & kCFSocketWriteCallBack) {
		if (xss->unreportedSent) {
			xss->unreportedSent = 0;
			xsBeginHost(the);
				xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(sizeof(xss->writeBuf)));
			xsEndHost(the);
		}
		else
			CFSocketDisableCallBacks(xss->cfSkt, kCFSocketWriteCallBack);
	}

	socketDownUseCount(the, xss);
}

int doFlushWrite(xsSocket xss)
{
	int ret;

	CFSocketEnableCallBacks(xss->cfSkt, kCFSocketReadCallBack | kCFSocketWriteCallBack);

	ret = write(xss->skt, xss->writeBuf, xss->writeBytes);
	if (ret < 0)
		return -1;

	modInstrumentationAdjust(NetworkBytesWritten, ret);

	if (ret > 0) {
		if (ret < xss->writeBytes)
			c_memcpy(xss->writeBuf, xss->writeBuf + ret, xss->writeBytes - ret);
		xss->writeBytes -= ret;
		xss->unreportedSent += ret;
	}

	return 0;
}

static void listenerCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	xsListener xsl = context;

	if (kCFSocketAcceptCallBack == cbType) {
		xsSocket xss = c_calloc(sizeof(xsSocketRecord), 1);

		xss->the = xsl->the;
		xss->useCount = 1;
		xss->skt = *(int *)data;

		CFSocketContext socketCtxt = {0, xss, NULL, NULL, NULL};
		xss->cfSkt = CFSocketCreateWithNative(kCFAllocatorDefault, xss->skt, kCFSocketConnectCallBack | kCFSocketReadCallBack | kCFSocketWriteCallBack, socketCallback, &socketCtxt);

		xsl->pending = xss;

		xsBeginHost(xsl->the);
		xsCall1(xsl->obj, xsID_callback, xsInteger(kListenerMsgConnect));
		xsEndHost(xsl->the);

		xsl->pending = NULL;
	}
}

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	struct sockaddr_in address;
	CFSocketError cfsErr;
	CFSocketContext context;
	uint16_t port = 0;

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}

	xsl = c_calloc(sizeof(xsListenerRecord), 1);
	xsl->the = the;
	xsl->obj = xsThis;
	xsmcSetHostData(xsThis, xsl);
	xsRemember(xsl->obj);

	c_memset(&context, 0, sizeof(CFSocketContext));
	context.info = (void*)xsl;
	xsl->cfSkt = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketAcceptCallBack, listenerCallback, &context);
	if (NULL == xsl->cfSkt)
		xsUnknownError("can't create socket");

    int yes = 1;
    setsockopt(CFSocketGetNative(xsl->cfSkt), SOL_SOCKET, SO_NOSIGPIPE, (void *)&yes, sizeof(yes));
    setsockopt(CFSocketGetNative(xsl->cfSkt), SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

	c_memset(&address, 0, sizeof(address));
	address.sin_len = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	CFDataRef data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&address, sizeof(address));
	cfsErr = CFSocketSetAddress(xsl->cfSkt, data);
	CFRelease(data);

	if (kCFSocketSuccess != cfsErr)
		xsUnknownError("bind failed");

	xsl->cfRunLoopSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, xsl->cfSkt, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), xsl->cfRunLoopSource, kCFRunLoopCommonModes);
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	if (xsl) {
		if (xsl->cfRunLoopSource) {
			CFRunLoopRemoveSource(CFRunLoopGetCurrent(), xsl->cfRunLoopSource, kCFRunLoopCommonModes);
			CFRelease(xsl->cfRunLoopSource);
		}
		if (xsl->cfSkt) {
			CFSocketInvalidate(xsl->cfSkt);
			CFRelease(xsl->cfSkt);
		}
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

