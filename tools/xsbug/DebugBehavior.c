/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "piuAll.h"
#if mxLinux
#include <gio/gio.h>
#elif mxMacOSX
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#elif mxWindows
#endif

#define XS_BUFFER_COUNT 1023
enum {
	XS_BODY_STATE,
	XS_DATA_STATE,
	XS_TAG_STATE,
	XS_START_TAG_NAME_STATE,
	XS_START_TAG_SPACE_STATE,
	XS_ATTRIBUTE_NAME_STATE,
	XS_ATTRIBUTE_SPACE_STATE,
	XS_ATTRIBUTE_EQUAL_STATE,
	XS_ATTRIBUTE_VALUE_STATE,
	XS_EMPTY_TAG_STATE,
	XS_END_TAG_STATE,
	XS_END_TAG_NAME_STATE,
	XS_END_TAG_SPACE_STATE,
	XS_PROCESSING_INSTRUCTION_STATE,
	XS_PROCESSING_INSTRUCTION_SPACE_STATE,
	XS_ENTITY_STATE,
	XS_ENTITY_NUMBER_STATE,
	XS_ERROR_STATE
};

typedef struct PiuDebugBehaviorStruct PiuDebugBehaviorRecord, *PiuDebugBehavior;
typedef struct PiuDebugMachineStruct PiuDebugMachineRecord, *PiuDebugMachine;

struct PiuDebugBehaviorStruct {
	xsMachine* the;
	xsSlot thisSlot;
#if mxLinux
	GSocket* socket;
	GSource* source;
#elif mxMacOSX
	CFSocketRef socket;
	CFRunLoopSourceRef networkSource;
#elif mxWindows
	SOCKET socket;
	HWND window;
#endif
};

struct PiuDebugMachineStruct {
	xsMachine* the;
	xsSlot thisSlot;
	xsSlot itemSlot;
	xsSlot listSlot;
#if mxLinux
	GSocket* socket;
	GSource* source;
#elif mxMacOSX
	CFSocketRef socket;
	CFRunLoopSourceRef networkSource;
#elif mxWindows
	SOCKET socket;
	HWND window;
#endif
	int column;
	int logging;
	int state;
	
	int bufferIndex;
	int attributeIndex;
	int dataIndex;
	int tagIndex;
	int entityNumber;
	int entityState;

	char buffer[XS_BUFFER_COUNT + 1];
	char attribute[XS_BUFFER_COUNT + 1];
	char data[XS_BUFFER_COUNT + 1];
	char tag[XS_BUFFER_COUNT + 1];
};

#if mxLinux
static gboolean PiuDebugBehaviorCallback(GSocket *socket, GIOCondition condition, gpointer user_data);
#elif mxMacOSX
static void PiuDebugBehaviorCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
#elif mxWindows
static LRESULT CALLBACK PiuDebugBehaviorWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
#endif
static void PiuDebugBehaviorMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuDebugBehaviorStart(PiuDebugBehavior self, int port);
static void PiuDebugBehaviorStop(PiuDebugBehavior self);

#if mxLinux
static gboolean PiuDebugMachineCallback(GSocket *socket, GIOCondition condition, gpointer user_data);
#elif mxMacOSX
static void PiuDebugMachineCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
#elif mxWindows
static LRESULT CALLBACK PiuDebugMachineWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
#endif
static void PiuDebugMachineDisconnect(PiuDebugMachine self);
static void PiuDebugMachineMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuDebugMachineParse(PiuDebugMachine self, char* theString, int theLength);
static void PiuDebugMachineParseAttribute(PiuDebugMachine self, char* theName, char* theValue);
static void PiuDebugMachineParseData(PiuDebugMachine self, char* theData);
static void PiuDebugMachineParseString(PiuDebugMachine self, char* theString);
static void PiuDebugMachineParseTag(PiuDebugMachine self, char* theName);
static void PiuDebugMachinePopTag(PiuDebugMachine self, char* theName);
static void PiuDebugMachinePushTag(PiuDebugMachine self, char* theName);

enum {
	mxFramesView = 0,
	mxLocalsView,
	mxGlobalsView,
	mxFilesView,
	mxBreakpointsView,
	mxGrammarsView,
	mxInstrumentsView,
	mxViewCount,
};

enum {
	mxAbortCommand = 0,
	mxClearAllBreakpoints,
	mxClearBreakpoint,
	mxGoCommand,
	mxLogoutCommand,
	mxSelectCommand,
	mxSetAllBreakpointsCommand,
	mxSetBreakpointCommand,
	mxStepCommand,
	mxStepInCommand,
	mxStepOutCommand,
	mxToggleCommand,
};

static xsHostHooks PiuDebugBehaviorHooks = {
	PiuDebugBehaviorDelete,
	PiuDebugBehaviorMark,
	NULL
};

static xsHostHooks PiuDebugMachineHooks = {
	PiuDebugMachineDelete,
	PiuDebugMachineMark,
	NULL
};

#if mxLinux
gboolean PiuDebugBehaviorCallback(GSocket *socket, GIOCondition condition, gpointer user_data)
{
	PiuDebugBehavior self = (PiuDebugBehavior)user_data;
	xsBeginHost(self->the);
	{
		xsResult = xsCall1(self->thisSlot, xsID_onAccepted, self->thisSlot);
	}
	xsEndHost();
	return TRUE;
}
#elif mxMacOSX
void PiuDebugBehaviorCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	if (cbType == kCFSocketAcceptCallBack) {
		PiuDebugBehavior self = context;
		int fd = *((int*)data);
		xsBeginHost(self->the);
		{
			xsResult = xsCall1(self->thisSlot, xsID_onAccepted, xsInteger(fd));
		}
		xsEndHost();
	}
}
#elif mxWindows
LRESULT CALLBACK PiuDebugBehaviorWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER) {
		PiuDebugBehavior self = (PiuDebugBehavior)GetWindowLongPtr(window, 0);
		xsBeginHost(self->the);
		{
			xsResult = xsCall1(self->thisSlot, xsID_onAccepted, self->thisSlot);
		}
		xsEndHost(self->the);
		return TRUE;
	}
	return DefWindowProc(window, message, wParam, lParam);
}
#endif

void PiuDebugBehaviorCreate(xsMachine* the)
{
	PiuDebugBehavior self;
	self = c_calloc(1, sizeof(PiuDebugBehaviorRecord));
	if (!self)
		xsUnknownError("not enough memory");
	self->the = the;
	self->thisSlot = xsThis;
	xsSetHostData(xsThis, self);
	xsSetHostHooks(xsThis, &PiuDebugBehaviorHooks);
#if mxWindows
	WNDCLASSEX wcex;
	WSADATA wsaData;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(PiuDebugBehavior);
	wcex.lpfnWndProc = PiuDebugBehaviorWindowProc;
	wcex.lpszClassName = "PiuDebugBehaviorWindow";
	RegisterClassEx(&wcex);
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(PiuDebugMachine);
	wcex.lpfnWndProc = PiuDebugMachineWindowProc;
	wcex.lpszClassName = "PiuDebugMachineWindow";
	RegisterClassEx(&wcex);
	int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	self->socket = INVALID_SOCKET;
#endif
}

void PiuDebugBehaviorDelete(void* data)
{
	PiuDebugBehavior self = data;
	if (self) {
		PiuDebugBehaviorStop(self);
	#if mxWindows
		WSACleanup();
	#endif
		c_free(self);
	}
}

void PiuDebugBehaviorMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDebugBehavior self = it;
	(*markRoot)(the, &(self->thisSlot));
}

void PiuDebugBehaviorStart(PiuDebugBehavior self, int port)
{
#if mxLinux
	xsMachine* the = self->the;
	GError* error = NULL;
	GSocketAddress* address = g_inet_socket_address_new(g_inet_address_new_any(G_SOCKET_FAMILY_IPV4), port);
  	self->socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, 0, &error);
  	if (!self->socket)
  		xsUnknownError("g_socket_new: %s\n", error->message);
  	if (!g_socket_bind(self->socket, address, TRUE, &error))
  		xsUnknownError("g_socket_bind: %s\n", error->message);
 	g_object_unref(address);
 		
    g_socket_set_blocking(self->socket, FALSE);
  	self->source = g_socket_create_source(self->socket, G_IO_IN, NULL);
	g_source_set_callback(self->source, (void*)PiuDebugBehaviorCallback, self, NULL);
	g_source_set_priority(self->source, G_PRIORITY_DEFAULT);
	g_source_attach(self->source, g_main_context_get_thread_default());
  	if (!g_socket_listen(self->socket, &error))
  		xsUnknownError("g_socket_listen: %s\n", error->message);
#elif mxMacOSX
	struct sockaddr_in address;
	CFSocketContext context;

	c_memset(&context, 0, sizeof(CFSocketContext));
	context.info = (void*)self;
	self->socket = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketAcceptCallBack, PiuDebugBehaviorCallback, &context);

    int yes = 1;
    setsockopt(CFSocketGetNative(self->socket), SOL_SOCKET, SO_NOSIGPIPE, (void *)&yes, sizeof(yes));
    setsockopt(CFSocketGetNative(self->socket), SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

	memset(&address, 0, sizeof(sin));
	address.sin_len = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	CFDataRef data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&address, sizeof(address));
	CFSocketSetAddress(self->socket, data);
	CFRelease(data);

	self->networkSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, self->socket, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), self->networkSource, kCFRunLoopCommonModes);
#elif mxWindows
	struct sockaddr_in address;
	self->window = CreateWindowEx(0, "PiuDebugBehaviorWindow", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(self->window, 0, (LONG)self);
	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	self->socket = socket(AF_INET, SOCK_STREAM, 0); 
	if (self->socket == INVALID_SOCKET) goto bail;
	if (bind(self->socket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) goto bail;
	if (listen(self->socket, 1) == SOCKET_ERROR) goto bail;
	WSAAsyncSelect(self->socket, self->window, WM_USER, FD_ACCEPT);
bail:
	return;
#endif
}

void PiuDebugBehaviorStop(PiuDebugBehavior self)
{
#if mxLinux
	if (self->source) {
		g_source_destroy(self->source);
		self->source = NULL;
	}
	if (self->socket) {
		g_socket_close(self->socket, NULL);
		g_object_unref(self->socket);
		self->socket = NULL;
	}
#elif mxMacOSX
	if (self->networkSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->networkSource, kCFRunLoopCommonModes);
		CFRelease(self->networkSource);
		self->networkSource = NULL;
	}
	if (self->socket) {
		CFSocketInvalidate(self->socket);
		CFRelease(self->socket);
		self->socket = NULL;
	}
#elif mxWindows
	if (self->socket) {
		closesocket(self->socket);
		self->socket = INVALID_SOCKET;
	}
	if (self->window) {
		DestroyWindow(self->window);
		self->window = NULL;
	}
#endif
}

void PiuDebugBehavior_start(xsMachine* the)
{
	PiuDebugBehavior self = xsGetHostData(xsThis);
	xsIntegerValue port = xsToInteger(xsGet(xsThis, xsID_port));
	PiuDebugBehaviorStart(self, port);
}

void PiuDebugBehavior_stop(xsMachine* the)
{
	PiuDebugBehavior self = xsGetHostData(xsThis);
	PiuDebugBehaviorStop(self);
}

void PiuDebugBehavior_formatBinaryMessage(xsMachine* the)
{
	xsIntegerValue length = c_strlen(xsToString(xsArg(0)));
	if (length) {
		xsStringValue from, to;
		xsResult = xsStringBuffer(NULL, length + (length / 2) - 1);
		from = xsToString(xsArg(0));
		to = xsToString(xsResult);
		for (;;) {
			*to++ = *from++;
			*to++ = *from++;
			if (*from)
				*to++ = ' ';
			else
				break;
		}
	}
	else
		xsResult = xsArg(0);
}

#if mxLinux
gboolean PiuDebugMachineCallback(GSocket *socket, GIOCondition condition, gpointer user_data)
{
	PiuDebugMachine self = (PiuDebugMachine)user_data;
	gssize length = g_socket_receive(self->socket, self->buffer, XS_BUFFER_COUNT, NULL, NULL);
	if (length > 0) {
		self->buffer[length] = 0;
		//fprintf(stderr, "%s", self->buffer);
		xsBeginHost(self->the);
		{
			xsVars(4);
			PiuDebugMachineParse(self, self->buffer, length);
		}
		xsEndHost();
	}
	else {
		PiuDebugMachineDisconnect(self);
		xsBeginHost(self->the);
		{
			(void)xsCall0(self->thisSlot, xsID_onDisconnected);
		}
		xsEndHost();
	}
	return TRUE;
}
#elif mxMacOSX
void PiuDebugMachineCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	if (cbType == kCFSocketReadCallBack) {
		PiuDebugMachine self = context;
		int length = recv(CFSocketGetNative(self->socket), self->buffer, XS_BUFFER_COUNT, 0);
		if (length > 0) {
			self->buffer[length] = 0;
			//fprintf(stderr, "%s", self->buffer);
			xsBeginHost(self->the);
			{
				xsVars(4);
				PiuDebugMachineParse(self, self->buffer, length);
			}
			xsEndHost();
		}
		else if (errno != EINTR) {
			PiuDebugMachineDisconnect(self);
			xsBeginHost(self->the);
			{
				(void)xsCall0(self->thisSlot, xsID_onDisconnected);
			}
			xsEndHost();
		}
	}
}
#elif mxWindows
LRESULT CALLBACK PiuDebugMachineWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER) {
		PiuDebugMachine self = (PiuDebugMachine)GetWindowLongPtr(window, 0);
		if (WSAGETSELECTEVENT(lParam) == FD_READ) {
			int length = recv(self->socket, self->buffer, XS_BUFFER_COUNT, 0);
			if (length > 0) {
				self->buffer[length] = 0;
				//fprintf(stderr, "%s", self->buffer);
				xsBeginHost(self->the);
				{
					xsVars(4);
					PiuDebugMachineParse(self, self->buffer, length);
				}
				xsEndHost(self->the);
				return TRUE;
			}
		}
		PiuDebugMachineDisconnect(self);
		xsBeginHost(self->the);
		{
			(void)xsCall0(self->thisSlot, xsID_onDisconnected);
		}
		xsEndHost(self->the);
		return TRUE;
	}
	return DefWindowProc(window, message, wParam, lParam);
}
#endif

void PiuDebugMachineCreate(xsMachine* the)
{
	PiuDebugMachine self;
    
	self = c_calloc(1, sizeof(PiuDebugMachineRecord));
	if (!self)
		xsUnknownError("not enough memory");
	self->the = the;
	self->thisSlot = xsThis;
	self->itemSlot = xsUndefined;
	self->listSlot = xsUndefined;
	xsSetHostData(xsThis, self);
	xsSetHostHooks(xsThis, &PiuDebugMachineHooks);
#if mxLinux
	{	
		PiuDebugBehavior behavior = xsGetHostData(xsArg(0));
		GError* error = NULL;
		GSocketAddress *address = NULL;
		const guint8* ip;
		int port;
		char buffer[22];

		self->socket = g_socket_accept(behavior->socket, NULL, &error);
		if (!self->socket)
			xsUnknownError("g_socket_accept %s", error->message);
			
		address = g_socket_get_remote_address(self->socket, &error);
		if (!address)
			xsUnknownError("g_socket_get_remote_address %s", error->message);
		ip = g_inet_address_to_bytes(g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(address)));
		port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(address));
		snprintf(buffer, 22, "%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], port);
		xsSet(xsThis, xsID_address, xsString(buffer));
		g_object_unref(address);
		address = NULL;
		
		g_socket_set_blocking(self->socket, FALSE);
		self->source = g_socket_create_source(self->socket, G_IO_IN, NULL);
		g_source_set_callback(self->source, (void*)PiuDebugMachineCallback, self, NULL);
		g_source_set_priority(self->source, G_PRIORITY_DEFAULT);
		g_source_attach(self->source, g_main_context_get_thread_default());
	}
#elif mxMacOSX
	{	
		int fd;
		unsigned long flag;
		socklen_t length;
		struct sockaddr_in address;
		int ip, port;
		char buffer[22];
		CFSocketContext context;

		fd = xsToInteger(xsArg(0));
		flag = fcntl(fd, F_GETFL, 0);
		if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
			xsUnknownError("fcntl error: %s", strerror(errno));

		length = sizeof(struct sockaddr_in);
		if (getpeername(fd, (struct sockaddr*)(void*)&address, &length) == -1)
			xsUnknownError("getpeername error: %s", strerror(errno));
		ip = ntohl(address.sin_addr.s_addr);
		port = ntohs(address.sin_port);
		snprintf(buffer, 22, "%u.%u.%u.%u:%u", (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff), port);
		xsSet(xsThis, xsID_address, xsString(buffer));
	
		c_memset(&context, 0, sizeof(CFSocketContext));
		context.info = (void*)self;
		self->socket = CFSocketCreateWithNative(NULL, fd, kCFSocketReadCallBack, PiuDebugMachineCallback, &context);
		self->networkSource = CFSocketCreateRunLoopSource(NULL, self->socket, 0);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), self->networkSource, kCFRunLoopCommonModes);
	}
#elif mxWindows
	{	
		PiuDebugBehavior behavior = xsGetHostData(xsArg(0));
		struct sockaddr_in address;
		int size = sizeof(address);
		int ip, port;
		char buffer[22];
		self->window = CreateWindowEx(0, "PiuDebugMachineWindow", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
		SetWindowLongPtr(self->window, 0, (LONG)self);
		self->socket = accept(behavior->socket, (struct sockaddr*)&address, &size);
		ip = ntohl(address.sin_addr.s_addr);
		port = ntohs(address.sin_port);
		snprintf(buffer, 22, "%u.%u.%u.%u:%u", (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff), port);
		xsSet(xsThis, xsID_address, xsString(buffer));
		WSAAsyncSelect(self->socket, self->window, WM_USER, FD_CLOSE | FD_READ);
	}
#endif
}

void PiuDebugMachineDelete(void* data)
{
	PiuDebugMachine self = data;
	if (self) {
		PiuDebugMachineDisconnect(self);
		c_free(self);
	}
}

void PiuDebugMachineDisconnect(PiuDebugMachine self)
{
#if mxLinux
	if (self->source) {
		g_source_destroy(self->source);
		self->source = NULL;
	}
	if (self->socket) {
		g_socket_close(self->socket, NULL);
		g_object_unref(self->socket);
		self->socket = NULL;
	}
#elif mxMacOSX
	if (self->networkSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->networkSource, kCFRunLoopCommonModes);
		CFRelease(self->networkSource);
		self->networkSource = NULL;
	}
	if (self->socket) {
		CFSocketInvalidate(self->socket);
		CFRelease(self->socket);
		self->socket = NULL;
	}
#elif mxWindows
	if (self->socket != INVALID_SOCKET) {
		closesocket(self->socket);
		self->socket = INVALID_SOCKET;
	}
	if (self->window) {
		DestroyWindow(self->window);
		self->window = NULL;
	}
#endif
}

void PiuDebugMachineMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDebugMachine self = it;
	(*markRoot)(the, &(self->thisSlot));
	(*markRoot)(the, &(self->itemSlot));
	(*markRoot)(the, &(self->listSlot));
}


#define mxIsDigit(c) \
	(('0' <= c) && (c <= '9'))
#define mxIsFirstLetter(c) \
	((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':'))
#define mxIsNextLetter(c) \
	((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '.') || (c == '-') || (c == '_') || (c == ':'))
#define mxIsSpace(c) \
	((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))

void PiuDebugMachineParse(PiuDebugMachine self, char* theString, int theLength)
{
	char c;
	while (theLength) {
		c = *theString++;
		switch (self->state) {				
		case XS_BODY_STATE:
			if (c == '<')
				self->state = XS_TAG_STATE;
			break;
				
		case XS_DATA_STATE:
			if (c == '<') {
				self->data[self->dataIndex] = 0;
				PiuDebugMachineParseString(self, self->data);
				PiuDebugMachineParseData(self, self->data);
				self->dataIndex = 0;
				self->state = XS_TAG_STATE;
			}
			else if (c == '&') {
				self->entityState = XS_DATA_STATE;
				self->state = XS_ENTITY_STATE;
			}
			else {
				if (self->dataIndex == XS_BUFFER_COUNT) {
					self->data[self->dataIndex] = 0;
					PiuDebugMachineParseString(self, self->data);
					PiuDebugMachineParseData(self, self->data);
					self->dataIndex = 0;
				}
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			break;
				
		case XS_TAG_STATE:
			if (c == '/')
				self->state = XS_END_TAG_STATE;
			else if (c == '?')
				self->state = XS_PROCESSING_INSTRUCTION_STATE;
			else if (mxIsFirstLetter(c)) {
				self->state = XS_START_TAG_NAME_STATE;
				self->tag[0] = c;
				self->tagIndex = 1;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_START_TAG_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {	
				self->tag[self->tagIndex] = c;
				self->tagIndex++;
				break;
			}
			else {
				self->tag[self->tagIndex] = 0;
				PiuDebugMachinePushTag(self, self->tag);
				self->state = XS_START_TAG_SPACE_STATE;
				/* continue */
			}
		case XS_START_TAG_SPACE_STATE:
			if (mxIsFirstLetter(c)) {
				self->state = XS_ATTRIBUTE_NAME_STATE;
				self->attribute[0] = c;
				self->attributeIndex = 1;
			}
			else if (c == '/')
				self->state = XS_EMPTY_TAG_STATE;
			else if (c == '>') {
				PiuDebugMachineParseTag(self, self->tag);
				self->state = XS_DATA_STATE;
				self->dataIndex = 0;
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ATTRIBUTE_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {	
				self->attribute[self->attributeIndex] = c;
				self->attributeIndex++;
				break;
			}
			else {
				self->attribute[self->attributeIndex] = 0;
				self->state = XS_ATTRIBUTE_SPACE_STATE;
				/* continue */
			}
		case XS_ATTRIBUTE_SPACE_STATE:
			if (c == '=')
				self->state = XS_ATTRIBUTE_EQUAL_STATE;
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_EQUAL_STATE:
			if (c == '"') {
				self->state = XS_ATTRIBUTE_VALUE_STATE;
				self->dataIndex = 0;
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_VALUE_STATE:
			if (c == '"') {
				self->data[self->dataIndex] = 0;
				PiuDebugMachineParseString(self, self->data);
				PiuDebugMachineParseAttribute(self, self->attribute, self->data);
				self->state = XS_START_TAG_SPACE_STATE;
			}
			else if (c == '&') {
				self->entityState = XS_ATTRIBUTE_VALUE_STATE;
				self->state = XS_ENTITY_STATE;
			}
			else if (self->dataIndex < XS_BUFFER_COUNT) {
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			else {
				//self->state = XS_ERROR_STATE;
			}
			break;
			
		case XS_EMPTY_TAG_STATE:
			if (c == '>') {
				self->state = XS_BODY_STATE;
				PiuDebugMachineParseTag(self, self->tag);
				PiuDebugMachinePopTag(self, self->tag);
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_END_TAG_STATE:
			if (mxIsFirstLetter(c))	{
				self->state = XS_END_TAG_NAME_STATE;
				self->tag[0] = c;
				self->tagIndex = 1;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
		case XS_END_TAG_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {
				self->tag[self->tagIndex] = c;
				self->tagIndex++;
				break;
			}
			else {
				self->tag[self->tagIndex] = 0;
				self->state = XS_END_TAG_SPACE_STATE;
				/* continue */
			}
		case XS_END_TAG_SPACE_STATE:
			if (c == '>') {
				self->state = XS_BODY_STATE;
				PiuDebugMachinePopTag(self, self->tag);
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;

		case XS_PROCESSING_INSTRUCTION_STATE:
			if (c == '?')
				self->state = XS_PROCESSING_INSTRUCTION_SPACE_STATE;
			break;
		case XS_PROCESSING_INSTRUCTION_SPACE_STATE:
			if (c == '>')
				self->state = XS_BODY_STATE;
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ENTITY_STATE:
			if (c == '#') {
				self->entityNumber = 0;
				self->state = XS_ENTITY_NUMBER_STATE;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
		case XS_ENTITY_NUMBER_STATE:
			if (mxIsDigit(c))
				self->entityNumber = (self->entityNumber * 10) + (c - '0');
			else if (c == ';') {
				if (self->dataIndex == XS_BUFFER_COUNT) {
					self->data[self->dataIndex] = 0;
					PiuDebugMachineParseData(self, self->data);
					self->dataIndex = 0;
				}
				self->data[self->dataIndex] = (char)self->entityNumber;
				self->dataIndex++;
				self->state = self->entityState;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ERROR_STATE:
			//fprintf(stderr, "\n### ERROR: %c\n", c);
			break;
		}
		theLength--;
	}
}

void PiuDebugMachineParseAttribute(PiuDebugMachine self, char* theName, char* theValue)
{
	xsMachine* the = self->the;
	if (strcmp(theName, "flags") == 0) {
		xsDefine(self->itemSlot, xsID_flags, xsString(theValue + 1), xsDefault);
		if (theValue[0] == ' ')
			xsDefine(self->itemSlot, xsID_state, xsInteger(0), xsDefault);
		else if (theValue[0] == '+')
			xsDefine(self->itemSlot, xsID_state, xsInteger(1), xsDefault);
		else if (theValue[0] == '-')
			xsDefine(self->itemSlot, xsID_state, xsInteger(3), xsDefault);
		else
			xsDefine(self->itemSlot, xsID_state, xsInteger(0), xsDefault);
			
		if (theValue[4] == 'I')
			xsDefine(self->itemSlot, xsID_variant, xsInteger(2), xsDefault);
		else if (theValue[4] == 'M')
			xsDefine(self->itemSlot, xsID_variant, xsInteger(1), xsDefault);
		else
			xsDefine(self->itemSlot, xsID_variant, xsInteger(0), xsDefault);
	}
	else if (strcmp(theName, "line") == 0)
		xsDefine(self->itemSlot, xsID_line, xsInteger(strtol(theValue, NULL, 10)), xsDefault);
	else if (strcmp(theName, "name") == 0)
		xsDefine(self->itemSlot, xsID_name, xsString(theValue), xsDefault);
	else if (strcmp(theName, "path") == 0)
		xsDefine(self->itemSlot, xsID_path, xsString(theValue), xsDefault);
	else if (strcmp(theName, "value") == 0)
		xsDefine(self->itemSlot, xsID_value, xsString(theValue), xsDefault);
}

void PiuDebugMachineParseData(PiuDebugMachine self, char* theData)
{	
	xsMachine* the = self->the;
	if (self->logging) {
		xsResult = xsGet(self->itemSlot, xsID_data);
		xsResult = xsCall1(xsResult, xsID_concat, xsString(theData));
		xsSet(self->itemSlot, xsID_data, xsResult);
	}
}

typedef struct {
	txS2 size;
	txU1 cmask;
	txU1 cval;
} PiuDebugUTF8Sequence;

void PiuDebugMachineParseString(PiuDebugMachine self, char* theString)
{	
	static PiuDebugUTF8Sequence sequences[] = {
		{1, 0x80, 0x00},
		{2, 0xE0, 0xC0},
		{3, 0xF0, 0xE0},
		{4, 0xF8, 0xF0},
		{5, 0xFC, 0xF8},
		{6, 0xFE, 0xFC},
		{0, 0, 0},
	};
	PiuDebugUTF8Sequence* sequence;
	txU1* p = (txU1*)theString;
	txU4 c;
	txS2 s, i;
	while ((c = *p)) {
		for (sequence = sequences; sequence->size; sequence++) {
			if ((c & sequence->cmask) == sequence->cval)
				break;
		}
		s = sequence->size;
		if (s) {
			for (i = 1; i < s; i++) {
				txU1 d = p[i];
				if ((d < 0x80) || (0xBF < d))
					break;
				c = (c << 6) | (d & 0x3F);
			}
		}
		if ((i == s) && (c < 0x110000))
			p += s;
		else {
			*p = 0;
			break;
		}
	}
}

void PiuDebugMachineParseTag(PiuDebugMachine self, char* theName)
{
	xsMachine* the = self->the;
	if (strcmp(theName, "break") == 0) {
		xsDefine(self->itemSlot, xsID_data, xsString(""), xsDefault);
	}
	else if (strcmp(theName, "samples") == 0) {
		xsDefine(self->itemSlot, xsID_data, xsString(""), xsDefault);
	}
	else if (strcmp(theName, "log") == 0) {
		xsDefine(self->itemSlot, xsID_data, xsString(""), xsDefault);
	}
	else if (strcmp(theName, "bubble") == 0) {
		xsDefine(self->itemSlot, xsID_data, xsString(""), xsDefault);
	}
	else if (strcmp(theName, "breakpoint") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
	else if (strcmp(theName, "instrument") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
	else if (strcmp(theName, "file") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
	else if (strcmp(theName, "frame") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
	else if (strcmp(theName, "local") == 0) {
		xsVar(0) = xsGet(self->itemSlot, xsID_path);
		xsVar(1) = xsGet(self->itemSlot, xsID_line);
		if (xsTest(xsVar(0)) && xsTest(xsVar(1))) {
			(void)xsCall2(self->thisSlot, xsID_onFileChanged, xsVar(0), xsVar(1));
		}
		xsVar(0) = xsGet(self->itemSlot, xsID_name);
		xsVar(1) = xsGet(self->itemSlot, xsID_value);
		(void)xsCall2(self->thisSlot, xsID_onFrameChanged, xsVar(0), xsVar(1));
	}
	else if (strcmp(theName, "login") == 0) {
		xsVar(0) = xsGet(self->itemSlot, xsID_name);
		xsVar(1) = xsGet(self->itemSlot, xsID_value);
		(void)xsCall2(self->thisSlot, xsID_onTitleChanged, xsVar(0), xsVar(1));
	}
	else if (strcmp(theName, "node") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
	else if (strcmp(theName, "property") == 0) {
		(void)xsCall1(self->listSlot, xsID_push, self->itemSlot);
	}
}

void PiuDebugMachinePopTag(PiuDebugMachine self, char* theName)
{
	xsMachine* the = self->the;
	if (strcmp(theName, "xsbug") == 0) {
		(void)xsCall0(self->thisSlot, xsID_onParsed);
	}
	else if (strcmp(theName, "samples") == 0) {
        self->logging = 0;
		xsResult = xsGet(self->itemSlot, xsID_data);
		(void)xsCall1(self->thisSlot, xsID_onSampled, xsResult);
	}
	else if (strcmp(theName, "log") == 0) {
		self->logging = 0;
		xsVar(0) = xsGet(self->itemSlot, xsID_path);
		xsVar(1) = xsGet(self->itemSlot, xsID_line);
		xsResult = xsGet(self->itemSlot, xsID_data);
		(void)xsCall3(self->thisSlot, xsID_onLogged, xsVar(0), xsVar(1), xsResult);
	}
	else if (strcmp(theName, "bubble") == 0) {
        self->logging = 0;
		xsVar(0) = xsGet(self->itemSlot, xsID_path);
		xsVar(1) = xsGet(self->itemSlot, xsID_line);
		xsVar(2) = xsGet(self->itemSlot, xsID_name);
		xsVar(3) = xsGet(self->itemSlot, xsID_value);
		xsResult = xsGet(self->itemSlot, xsID_data);
		(void)xsCall5(self->thisSlot, xsID_onBubbled, xsVar(0), xsVar(1), xsVar(2), xsVar(3), xsResult);
	}
	else if (strcmp(theName, "break") == 0) {
		self->logging = 0;
		xsVar(0) = xsGet(self->itemSlot, xsID_path);
		xsVar(1) = xsGet(self->itemSlot, xsID_line);
		xsResult = xsGet(self->itemSlot, xsID_data);
		(void)xsCall3(self->thisSlot, xsID_onLogged, xsVar(0), xsVar(1), xsResult);
		(void)xsCall3(self->thisSlot, xsID_onBroken, xsVar(0), xsVar(1), xsResult);
	}
	else if (strcmp(theName, "breakpoints") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxBreakpointsView), self->listSlot);
		self->listSlot = xsUndefined;
	}
	else if (strcmp(theName, "instruments") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxInstrumentsView), self->listSlot);
		self->listSlot = xsUndefined;
	}
	else if (strcmp(theName, "files") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxFilesView), self->listSlot);
		self->listSlot = xsUndefined;
	}
	else if (strcmp(theName, "frames") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxFramesView), self->listSlot);
		self->listSlot = xsUndefined;
	}	
	else if (strcmp(theName, "global") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxGlobalsView), self->listSlot);
		self->listSlot = xsUndefined;
	}
	else if (strcmp(theName, "grammar") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxGrammarsView), self->listSlot);
		self->listSlot = xsUndefined;
	}
	else if (strcmp(theName, "local") == 0) {
		(void)xsCall2(self->thisSlot, xsID_onViewChanged, xsInteger(mxLocalsView), self->listSlot);
		self->listSlot = xsUndefined;
	}	
	else if (strcmp(theName, "node") == 0) {
		self->column--;
	}
	else if (strcmp(theName, "property") == 0) {
		self->column--;
	}
	self->itemSlot = xsUndefined;
}

void PiuDebugMachinePushTag(PiuDebugMachine self, char* theName)
{
	xsMachine* the = self->the;
	self->itemSlot = xsNewObject();
	if (strcmp(theName, "xsbug") == 0) {
		(void)xsCall0(self->thisSlot, xsID_onParsing);
	}
	else if (strcmp(theName, "samples") == 0) {
		self->logging = 1;
	}
	else if (strcmp(theName, "log") == 0) {
		self->logging = 1;
	}
	else if (strcmp(theName, "bubble") == 0) {
		self->logging = 1;
	}
	else if (strcmp(theName, "break") == 0) {
		self->logging = 1;
	}
	else if (strcmp(theName, "breakpoints") == 0) {
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "instruments") == 0) {
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "files") == 0) {
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "frames") == 0) {
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "global") == 0) {
		self->column = -1;
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "grammar") == 0) {
		self->column = -1;
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "local") == 0) {
		self->column = -1;
		self->listSlot = xsNewArray(0);
	}
	else if (strcmp(theName, "node") == 0) {
		self->column++;
		xsDefine(self->itemSlot, xsID_column, xsInteger(self->column), xsDefault);
	}
	else if (strcmp(theName, "property") == 0) {
		self->column++;
		xsDefine(self->itemSlot, xsID_column, xsInteger(self->column), xsDefault);
	}
}

void PiuDebugMachine_close(xsMachine* the)
{
	PiuDebugMachine self = xsGetHostData(xsThis);
	PiuDebugMachineDelete(self);
	xsSetHostData(xsThis, NULL);
	(void)xsCall0(xsThis, xsID_onDisconnected);
}

void PiuDebugMachine_doCommand(xsMachine* the)
{
	static char buffer[16 * 1024];
	PiuDebugMachine self = xsGetHostData(xsThis);
	xsIntegerValue c = xsToInteger(xsArgc), i;
	c_strcpy(buffer, "\15\12");
	switch (xsToInteger(xsArg(0))) {
	case mxAbortCommand:
		c_strcat(buffer, "<abort/>");
		break;
	case mxClearAllBreakpoints:
		c_strcat(buffer, "<clear-all-breakpoints/>");
		break;
	case mxClearBreakpoint:
		c_strcat(buffer, "<clear-breakpoint path=\"");
		c_strcat(buffer, xsToString(xsArg(1)));
		c_strcat(buffer, "\" line=\"");
		c_strcat(buffer, xsToString(xsArg(2)));
		c_strcat(buffer, "\"/>");
		break;
	case mxGoCommand:
		c_strcat(buffer, "<go/>");
		xsSet(xsThis, xsID_broken, xsFalse);
		break;
	case mxLogoutCommand:
		c_strcat(buffer, "<logout/>");
		break;
	case mxSelectCommand:
		c_strcat(buffer, "<select id=\"");
		c_strcat(buffer, xsToString(xsArg(1)));
		c_strcat(buffer, "\"/>");
		break;
	case mxSetAllBreakpointsCommand:
		c_strcat(buffer, "<set-all-breakpoints>");
		if ((c > 2) && (xsTest(xsArg(2))))
			c_strcat(buffer, "<breakpoint path=\"start\" line=\"0\"/>");
		if ((c > 3) && (xsTest(xsArg(3))))
			c_strcat(buffer, "<breakpoint path=\"exceptions\" line=\"0\"/>");
		c = xsToInteger(xsGet(xsArg(1), xsID("length")));
		for (i = 0; i < c; i++) {
			xsResult = xsGetAt(xsArg(1), xsInteger(i));
			c_strcat(buffer, "<breakpoint path=\"");
			c_strcat(buffer, xsToString(xsGet(xsResult, xsID("path"))));
			c_strcat(buffer, "\" line=\"");
			c_strcat(buffer, xsToString(xsGet(xsResult, xsID("line"))));
			c_strcat(buffer, "\"/>");
		}
		c_strcat(buffer, "</set-all-breakpoints>");
		break;
	case mxSetBreakpointCommand:
		c_strcat(buffer, "<set-breakpoint path=\"");
		c_strcat(buffer, xsToString(xsArg(1)));
		c_strcat(buffer, "\" line=\"");
		c_strcat(buffer, xsToString(xsArg(2)));
		c_strcat(buffer, "\"/>");
		break;
	case mxStepCommand:
		c_strcat(buffer, "<step/>");
		xsSet(xsThis, xsID_broken, xsFalse);
		break;
	case mxStepInCommand:
		c_strcat(buffer, "<step-inside/>");
		xsSet(xsThis, xsID_broken, xsFalse);
		break;
	case mxStepOutCommand:
		c_strcat(buffer, "<step-outside/>");
		xsSet(xsThis, xsID_broken, xsFalse);
		break;
	case mxToggleCommand:
		c_strcat(buffer, "<toggle id=\"");
		c_strcat(buffer, xsToString(xsArg(1)));
		c_strcat(buffer, "\"/>");
		break;
	}
	c_strcat(buffer, "\15\12");
    //fprintf(stderr, "%s", buffer);
#if mxLinux
	{
		GError* error = NULL;
		if (g_socket_send(self->socket, buffer, c_strlen(buffer), NULL, &error) == -1) {
			xsUnknownError("g_socket_send: %s", error->message);
		}
	}
#elif mxMacOSX
again:
	if (send(CFSocketGetNative(self->socket), buffer, c_strlen(buffer), 0) == -1) {
		if (errno == EINTR)
			goto again;
		else {
			xsMachine* the = self->the;
			xsUnknownError("write error: %s", strerror(errno));
		}
	}
#elif mxWindows
	if (self->socket != INVALID_SOCKET) {
	again:
		int count = send(self->socket, buffer, c_strlen(buffer), 0);
		if (count < 0) {
			if (WSAEWOULDBLOCK == WSAGetLastError()) {
				WaitMessage();
				goto again;
			}
		}
	}
#endif
}
















