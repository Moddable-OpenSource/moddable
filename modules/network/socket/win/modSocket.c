/*
 * Copyright (c) 2016-2022 Moddable Tech, Inc.
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
#include "../socket/modSocket.h"

#define kTCP (0)
#define kUDP (1)
#define kSocketAsyncSelectEvents (FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE )
#define kListenerAsyncSelectEvents (FD_ACCEPT)

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

struct xsSocketRecord {
	xsSlot						obj;
	xsMachine					*the;
	HWND						window;

    SOCKET						skt;
	int							port;
	char						host[256];

	uint8_t						useCount;
	uint8_t						done;
	uint8_t						kind;		// kTCP or kUDP

	uint8_t						*readBuffer;
	int32_t						readBytes;

	int32_t						writeBytes;
	int32_t						unreportedSent;		// bytes sent to the socket but not yet reported to object as sent

	uint8_t						writeBuf[1024];
	uint8_t 					readBuf[1024];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

struct xsListenerRecord {
	xsListener			next;

	xsMachine			*the;
	xsSlot				obj;

	SOCKET				skt;
	HWND				window;
	xsSocket			pending;
};

static void createSocketWindow(xsSocket xss);
static void createListenerWindow(xsListener xsl);
static DWORD WINAPI hostResolveProc(LPVOID lpParameter);
static LRESULT CALLBACK modSocketWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK modListenerWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int doFlushWrite(xsSocket xss);
static int doReadSocket(xsMachine *the, xsSocket xss);
static void doDestructor(xsSocket xss);

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
	WSADATA wsaData;

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		xsUnknownError("winsock initialization failed");

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

		if (WSAAsyncSelect(xss->skt, xss->window, WM_CALLBACK, kSocketAsyncSelectEvents))
			xsUnknownError("async select failed");

		modInstrumentationAdjust(NetworkSockets, 1);

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

	if (kTCP == xss->kind) {
		if (xsmcHas(xsArg(0), xsID_port)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			xss->port = xsmcToInteger(xsVar(0));
		}
		else
			xsUnknownError("port required in dictionary");

		xss->skt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	else
		xss->skt = socket(PF_INET, SOCK_DGRAM, 0);

	if (INVALID_SOCKET == xss->skt)
		xsUnknownError("create socket failed");

	modInstrumentationAdjust(NetworkSockets, 1);
	xsRemember(xss->obj);

	createSocketWindow(xss);

	if (WSAAsyncSelect(xss->skt, xss->window, WM_CALLBACK, kSocketAsyncSelectEvents))
		xsUnknownError("async select failed");

	if (kUDP == xss->kind)
		return;

	if (xsmcHas(xsArg(0), xsID_host)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_host);
		xsmcToStringBuffer(xsVar(0), xss->host, sizeof(xss->host));
		CreateThread(NULL, 0, hostResolveProc, xss, 0, NULL);
	}
	else
		xsUnknownError("host required in dictionary");
}

void createSocketWindow(xsSocket xss)
{
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(xsSocket);
	wcex.lpfnWndProc = modSocketWindowProc;
	wcex.lpszClassName = "modSocketWindowClass";
	RegisterClassEx(&wcex);
	xss->window = CreateWindowEx(0, wcex.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(xss->window, 0, (LONG_PTR)xss);
}

void createListenerWindow(xsListener xsl)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(xsListener);
	wcex.lpfnWndProc = modListenerWindowProc;
	wcex.lpszClassName = "modListenerWindowClass";
	RegisterClassEx(&wcex);
	xsl->window = CreateWindowEx(0, wcex.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(xsl->window, 0, (LONG_PTR)xsl);
}

DWORD WINAPI hostResolveProc(LPVOID lpParameter)
{
	xsSocket xss = (xsSocket)lpParameter;
	struct hostent *host;
	struct sockaddr_in address = {0};

	host = gethostbyname(xss->host);
	if (NULL != host) {
		address.sin_family = AF_INET;
		c_memcpy(&(address.sin_addr), host->h_addr, host->h_length);
		address.sin_port = htons(xss->port);
		connect(xss->skt, (struct sockaddr*)&address, sizeof(address));
	}

	return 0;
}

int doReadSocket(xsMachine *the, xsSocket xss)
{
	int count = 0;
	int full = sizeof(xss->readBuf) == xss->readBytes;

	if (!full) {
		if (xss->readBytes)
			c_memcpy(xss->readBuf, xss->readBuffer, xss->readBytes);
		xss->readBuffer = xss->readBuf;
		count = recv(xss->skt, xss->readBuf + xss->readBytes, sizeof(xss->readBuf) - xss->readBytes, 0);
	}
	if ((count > 0) || full) {
		modInstrumentationAdjust(NetworkBytesRead, count);
		xss->readBytes += count;

		xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(xss->readBytes));
		xsEndHost(the);
	}

	return count;
}

LRESULT CALLBACK modSocketWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_CALLBACK: {
			DWORD error = WSAGETSELECTERROR(lParam);
			xsSocket xss = (xsSocket)GetWindowLongPtr(window, 0);
			if (NULL == xss)
				break;
			xsMachine *the = xss->the;
			xss->useCount += 1;
			switch (WSAGETSELECTEVENT(lParam)) {
				case FD_CONNECT: {
					xsBeginHost(the);
					if (!error) {
						xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
					} else {
						xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));
					}
					xsEndHost(the);
				} break;
				case FD_CLOSE: {
					// read any remaining bytes buffered in socket before notifying disconnect
					int count;
					do {
						count = doReadSocket(the, xss);
					} while (count > 0);

					if (!xss->done) {
						xsBeginHost(the);
						xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
						xsEndHost(the);
					}
				} break;
				case FD_READ: {
					doReadSocket(the, xss);
				} break;
				case FD_WRITE: {
					if (xss->unreportedSent) {
						xss->unreportedSent = 0;
						xsBeginHost(the);
						xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(sizeof(xss->writeBuf)));
						xsEndHost(the);
					}
					else
						WSAAsyncSelect(xss->skt, xss->window, WM_CALLBACK, kSocketAsyncSelectEvents & ~FD_WRITE);
				} break;
			}
			socketDownUseCount(the, xss);
		} break;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

void doDestructor(xsSocket xss)
{
	xsMachine *the = xss->the;
	xss->done = 1;

	if (xss->skt != INVALID_SOCKET) {
		modInstrumentationAdjust(NetworkSockets, -1);
		WSAAsyncSelect(xss->skt, xss->window, WM_CALLBACK, 0);
		DestroyWindow(xss->window);
		closesocket(xss->skt);
		xss->skt = INVALID_SOCKET;
		WSACleanup();
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
		dest_addr.sin_addr.s_addr = inet_addr(temp);

		int result = sendto(xss->skt, buf, len, 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (result < 0)
			xsUnknownError("sendto failed");
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

	WSAAsyncSelect(xss->skt, xss->window, WM_CALLBACK, kSocketAsyncSelectEvents);

	ret = send(xss->skt, xss->writeBuf, xss->writeBytes, 0);
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

void xs_socket_suspend(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	uint16_t port = 0;
	WSADATA wsaData;
	struct sockaddr_in address = { 0 };

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16_t)xsmcToInteger(xsVar(0));
	}

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		xsUnknownError("winsock initialization failed");

	xsl = c_calloc(sizeof(xsListenerRecord), 1);

	xsl->skt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == xsl->skt)
		xsUnknownError("create socket failed");

	xsl->the = the;
	xsl->obj = xsThis;
	createListenerWindow(xsl);
	xsmcSetHostData(xsThis, xsl);
	xsRemember(xsl->obj);

	int yes = 1;
	setsockopt(xsl->skt, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);
	if (SOCKET_ERROR == bind(xsl->skt, (PSOCKADDR)&address, sizeof(address)))
		xsUnknownError("bind socket failed");

	if (WSAAsyncSelect(xsl->skt, xsl->window, WM_CALLBACK, kListenerAsyncSelectEvents))
		xsUnknownError("async select failed");

	if (SOCKET_ERROR == listen(xsl->skt, SOMAXCONN))
		xsUnknownError("listen failed");

	modInstrumentationAdjust(NetworkSockets, 1);
}

LRESULT CALLBACK modListenerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_CALLBACK: {
			if (!WSAGETSELECTERROR(lParam)) {
				xsListener xsl = (xsListener)GetWindowLongPtr(window, 0);
				xsMachine *the = xsl->the;
				switch (WSAGETSELECTEVENT(lParam)) {
					case FD_ACCEPT: {
						SOCKET skt = accept(wParam, NULL, NULL);
						if (INVALID_SOCKET != skt) {
							xsSocket xss = c_calloc(sizeof(xsSocketRecord), 1);
							xss->the = xsl->the;
							xss->useCount = 1;
							xss->skt = skt;
							createSocketWindow(xss);
							xsl->pending = xss;

							xsBeginHost(the);
							xsCall1(xsl->obj, xsID_callback, xsInteger(kListenerMsgConnect));
							xsEndHost(the);

							xsl->pending = NULL;
						}
					} break;
				}
			}
		} break;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	if (xsl) {
		DestroyWindow(xsl->window);
		if (xsl->skt != INVALID_SOCKET)
			closesocket(xsl->skt);
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

