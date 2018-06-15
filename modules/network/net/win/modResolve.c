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

typedef struct {
	xsSlot			callback;
	xsMachine		*the;
	HWND			window;
	char			ip[20];
	char			name[1];
} xsNetResolveRecord, *xsNetResolve;

static DWORD WINAPI hostResolveProc(LPVOID lpParameter);
static LRESULT CALLBACK modResolveWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void xs_net_resolve(xsMachine *the)
{
	xsNetResolve nr;
	char *name = xsToString(xsArg(0));
	int nameLen = c_strlen(name);
	HWND window;
	WSADATA wsaData;
	WNDCLASSEX wcex = { 0 };

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		xsUnknownError("winsock initialization failed");

	nr = c_calloc(sizeof(xsNetResolveRecord) + nameLen, 1);
	if (!nr)
		xsUnknownError("out of memory");

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = modResolveWindowProc;
	wcex.lpszClassName = "modResolveWindowClass";
	RegisterClassEx(&wcex);
	window = CreateWindowEx(0, wcex.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	nr->the = the;
	nr->window = window;
	nr->callback = xsArg(1);
	xsRemember(nr->callback);

	xsToStringBuffer(xsArg(0), nr->name, nameLen + 1);

	CreateThread(NULL, 0, hostResolveProc, nr, 0, NULL);
}

DWORD WINAPI hostResolveProc(LPVOID lpParameter)
{
	xsNetResolve nr = (xsNetResolve)lpParameter;
	struct hostent *host;

	host = gethostbyname(nr->name);
	if (host && (host->h_addrtype == AF_INET)) {
		struct in_addr addr;
		addr.s_addr = *(u_long *)host->h_addr_list[0];
		c_strcpy(nr->ip, inet_ntoa(addr));
	}
	PostMessage(nr->window, WM_CALLBACK, 0, (LPARAM)nr);

	return 0;
}

LRESULT CALLBACK modResolveWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_CALLBACK: {
			xsNetResolve nr = (xsNetResolve)lParam;
			xsMachine *the = nr->the;
			xsBeginHost(the);
			if (c_strlen(nr->ip))
				xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsString(nr->ip));
			else
				xsCallFunction1(nr->callback, xsGlobal, xsString(nr->name));
			xsEndHost(the);
			xsForget(nr->callback);
			DestroyWindow(nr->window);
			c_free(nr);
			WSACleanup();
		} break;
	}
	return DefWindowProc(window, message, wParam, lParam);
}
