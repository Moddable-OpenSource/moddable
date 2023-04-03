/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

#include "xsAll.h"
#include "xsScript.h"

#ifndef mxMessageWindowClass
	#define mxMessageWindowClass "fxMessageWindowClass"
#endif

static LRESULT CALLBACK fxMessageWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK fxMessageWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)	{
	case WM_PROMISE: {
		txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
		fxRunPromiseJobs(the);
	} break;
	case WM_SERVICE: {
		txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
		(*the->threadCallback)(the->thread);
	} break;
	case WM_WORKER: {
		txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
		txWorkerJob* job;
		EnterCriticalSection(&(the->workerMutex));
		job = the->workerQueue;
		the->workerQueue = NULL;
		LeaveCriticalSection(&(the->workerMutex));
		while (job) {
			txWorkerJob* next = job->next;
			(*job->callback)(the, job);
			c_free(job);
			job = next;
		}	
	} break;
#ifdef mxDebug
	case WM_XSBUG: {
		txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
		if (fxIsReadable(the)) {
			fxDebugCommand(the);
			if (the->breakOnStartFlag) {
				fxBeginHost(the);
				fxDebugLoop(the, NULL, 0, "C: xsDebugger");
				fxEndHost(the);
			}
		}
	} break;
#endif
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
}

void fxCreateMachinePlatform(txMachine* the)
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(txMachine*);
	wcex.lpfnWndProc = fxMessageWindowProc;
	wcex.lpszClassName = mxMessageWindowClass;
	RegisterClassEx(&wcex);
	the->window = CreateWindowEx(0, mxMessageWindowClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(the->window, 0, (LONG_PTR)the);
	InitializeCriticalSection(&(the->workerMutex));
#ifdef mxDebug
	the->connection = INVALID_SOCKET;
#endif
	the->demarshall = fxDemarshall;
}

void fxDeleteMachinePlatform(txMachine* the)
{
	EnterCriticalSection(&(the->workerMutex));
	{
		txWorkerJob* job = the->workerQueue;
		while (job) {
			txWorkerJob* next = job->next;
			c_free(job);
			job = next;
		}
		the->workerQueue = NULL;
	}
	LeaveCriticalSection(&(the->workerMutex));
	DeleteCriticalSection(&(the->workerMutex));
#ifdef mxDebug
	the->connection = INVALID_SOCKET;
#endif
	if (the->window) {
		DestroyWindow(the->window);
		the->window = NULL;
	}
	UnregisterClass(mxMessageWindowClass, NULL);
}

void fxQueuePromiseJobs(txMachine* the)
{
	PostMessage(the->window, WM_PROMISE, 0, 0);
}

void fxQueueWorkerJob(void* machine, void* job)
{
	txMachine* the = machine;
	EnterCriticalSection(&(the->workerMutex));
	{
		txWorkerJob** address = &(the->workerQueue);
		txWorkerJob* former;
		while ((former = *address))
			address = &(former->next);
		*address = job;
	}
	LeaveCriticalSection(&(the->workerMutex));
	PostMessage(the->window, WM_WORKER, 0, 0);
}

#ifdef mxDebug
void fxConnect(txMachine* the)
{
	WSADATA wsaData;
	struct hostent *host;
	struct sockaddr_in address;
	char name[C_PATH_MAX];
	unsigned long flag;
	char *hostname = "localhost";
	if (getenv("XSBUG_HOST"))
		hostname = getenv("XSBUG_HOST"); 

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		goto bail;
	host = gethostbyname(hostname);
	if (!host)
		goto bail;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	memcpy(&(address.sin_addr), host->h_addr, host->h_length);
	if (GetModuleFileName(NULL, name, sizeof(name)) && strstr(name, "xsbug"))
		address.sin_port = htons(5003);
	else {
		int port = 5002;
		char *portStr = getenv("XSBUG_PORT");
		if (portStr)
			port = atoi(portStr);
		address.sin_port = htons(port);
	}
	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection == INVALID_SOCKET)
		goto bail;
  	flag = 1;
  	ioctlsocket(the->connection, FIONBIO, &flag);
	if (connect(the->connection, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		if (WSAEWOULDBLOCK == WSAGetLastError()) {
			fd_set fds;
			struct timeval timeout = { 2, 0 }; // 2 seconds, 0 micro-seconds
			FD_ZERO(&fds);
			FD_SET(the->connection, &fds);
			if (select(0, NULL, &fds, NULL, &timeout) == 0)
				goto bail;
			if (!FD_ISSET(the->connection, &fds))
				goto bail;
		}
		else
			goto bail;
	}
 // flag = 0;
 //	ioctlsocket(the->connection, FIONBIO, &flag);
	if (WSAAsyncSelect(the->connection, the->window, WM_XSBUG, FD_READ))
		goto bail;
	return;
bail:
	fxDisconnect(the);
}

void fxDisconnect(txMachine* the)
{
	if (the->connection != INVALID_SOCKET) {
		closesocket(the->connection);
		the->connection = INVALID_SOCKET;
	}
	WSACleanup();
}

txBoolean fxIsConnected(txMachine* the)
{
	return (the->connection == INVALID_SOCKET) ? 0 : 1;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (the->connection != INVALID_SOCKET) {
		int count = recv(the->connection, the->debugBuffer, sizeof(the->debugBuffer) - 1, 0);
		if (count > 0) {
			the->debugOffset = count;
			return 1;
		}
	}
	return 0;
}

void fxReceive(txMachine* the)
{
	if (the->connection != INVALID_SOCKET) {
	again:
		if (0 == sizeof(the->debugBuffer) - the->debugOffset - 1)
			return;

		int count = recv(the->connection, the->debugBuffer + the->debugOffset, sizeof(the->debugBuffer) - the->debugOffset - 1, 0);
		if (count <= 0) {
			if (WSAEWOULDBLOCK == WSAGetLastError()) {
				if (the->debugOffset == 0)
					goto again;
			}
			else
				fxDisconnect(the);
		}
		else {
			the->debugOffset += count;
		}
		the->debugBuffer[the->debugOffset] = 0;
	}
}

void fxSend(txMachine* the, txBoolean more)
{
	if (the->connection != INVALID_SOCKET) {
	again:
		int count = send(the->connection, the->echoBuffer, the->echoOffset, 0);
		if (count < 0) {
			if (WSAEWOULDBLOCK == WSAGetLastError()) {
				WaitMessage();
				goto again;
			}
			fxDisconnect(the);
		}
	}
}
#endif
