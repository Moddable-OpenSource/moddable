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
 */

#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>

#define mxBufferSize 32 * 1024
#define mxMachinesCount 10
#define mxNetworkBufferSize 1024
#define mxSerialBufferSize 1024
#define mxTagSize 17
#define mxTrace 0

typedef struct txSerialMachineStruct txSerialMachineRecord, *txSerialMachine;
typedef struct txSerialToolStruct txSerialToolRecord, *txSerialTool;

struct txSerialMachineStruct {
	txSerialTool tool;
	txSerialMachine nextMachine;
	uint32_t value;
	SOCKET networkConnection;
	HANDLE networkEvent;
	WSAOVERLAPPED networkOverlapped;
	WSABUF networkBuf;
	char networkBuffer[mxNetworkBufferSize];
	char tag[mxTagSize + 1];
};

struct txSerialToolStruct {
	jmp_buf _jmp_buf;
	char* file;
	int line;
	DWORD error;
	
	HANDLE signalEvent;
	
	char* path;
	int baud;
	int data;
	int parity;
	int stop;
	HANDLE serialConnection;
	HANDLE serialEvent;
	OVERLAPPED serialOverlapped;
	char serialBuffer[mxSerialBufferSize];
	
	char* host;
	int port;
	DWORD count;
	HANDLE events[2 + mxMachinesCount];
	txSerialMachine firstMachine;
	txSerialMachine currentMachine;
	
	int index;
	int state;
	char buffer[mxBufferSize];
};


static void fxCloseNetwork(txSerialTool self, uint32_t value);
static void fxCloseSerial(txSerialTool self);
static void fxCountMachines(txSerialTool self);
static BOOL fxMatchProcessingInstruction(char* p, BOOL* flag, uint32_t* value);
static txSerialMachine fxOpenNetwork(txSerialTool self, uint32_t value);
static void fxOpenSerial(txSerialTool self);
static void fxReadNetwork(txSerialMachine machine, DWORD size);
static DWORD fxReadNetworkAux(txSerialMachine machine);
static void fxReadSerial(txSerialTool self, DWORD size);
static DWORD fxReadSerialAux(txSerialTool self);
static void fxWriteNetwork(txSerialTool self, char* buffer, DWORD size);
static void fxWriteSerial(txSerialTool self, char* buffer, DWORD size);

#define mxThrowError(_ERROR) (self->file=__FILE__, self->line=__LINE__, self->error=_ERROR, longjmp(self->_jmp_buf, 1))
#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = GetLastError(); longjmp(self->_jmp_buf, 1); } }
#define mxWSAThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = WSAGetLastError(); longjmp(self->_jmp_buf, 1); } }

void fxCloseNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if (machine->value == value)
			break;
		link = &(machine->nextMachine);
	}
	if (machine) {
		*link = machine->nextMachine;
		if (self->currentMachine == machine)
			self->currentMachine = NULL;
		
		CloseHandle(machine->networkOverlapped.hEvent);
		CloseHandle(machine->networkEvent);
		if (machine->networkConnection != INVALID_SOCKET)
			closesocket(machine->networkConnection);
		free(machine);
		
		fxCountMachines(self);
	}
}

void fxCloseSerial(txSerialTool self)
{
	if (self->serialConnection != INVALID_HANDLE_VALUE) {
		CloseHandle(self->serialConnection);
		self->serialConnection = INVALID_HANDLE_VALUE;
	}
}

void fxCountMachines(txSerialTool self)
{
	txSerialMachine machine = self->firstMachine;
	self->count = 2;
	while (machine) {
		self->events[self->count] = machine->networkOverlapped.hEvent;
		self->count++;
		machine = machine->nextMachine;
	}
}

BOOL fxMatchProcessingInstruction(char* p, BOOL* flag, uint32_t* value)
{
	char c;
	int i;
	if (*p++ != '<')
		return 0;
	if (*p++ != '?')
		return 0;
	if (*p++ != 'x')
		return 0;
	if (*p++ != 's')
		return 0;
	c = *p++;
	if (c == '.')
		*flag = 1;
	else if (c == '-')
		*flag = 0;
	else
		return 0;
	*value = 0;
	for (i = 0; i < 8; i++) {
		c = *p++;
		if (('0' <= c) && (c <= '9'))
			*value = (*value * 16) + (c - '0');
		else if (('a' <= c) && (c <= 'f'))
			*value = (*value * 16) + (10 + c - 'a');
		else if (('A' <= c) && (c <= 'F'))
			*value = (*value * 16) + (10 + c - 'A');
		else
			return 0;
	}
	if (*p++ != '?')
		return 0;
	if (*p++ != '>')
		return 0;
	return 1;
}

txSerialMachine fxOpenNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if (machine->value == value)
			break;
		link = &(machine->nextMachine);
	}
	if (!machine) {
		struct sockaddr_in address;
		
		if (self->count == 2 + mxMachinesCount)
			mxThrowError(ERROR_NOT_ENOUGH_MEMORY);
		
		machine = calloc(sizeof(txSerialMachineRecord), 1);
		mxThrowElse(machine != NULL);
		machine->tool = self;
		machine->value = value;
		sprintf(machine->tag, "?xs.%8.8X?>\r\n<", value);
		*link = machine;
		
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(self->host);
		if (address.sin_addr.s_addr == INADDR_NONE) {
			struct hostent *host = gethostbyname(self->host);
			mxWSAThrowElse(host);
			memcpy(&(address.sin_addr), host->h_addr, host->h_length);
		}
		address.sin_port = htons(self->port);

		machine->networkConnection = socket(AF_INET, SOCK_STREAM, 0);
		mxWSAThrowElse(machine->networkConnection != INVALID_SOCKET);
		mxWSAThrowElse(!connect(machine->networkConnection, (struct sockaddr*)&address, sizeof(address)));
	
		machine->networkEvent = WSACreateEvent();
		mxThrowElse(machine->networkEvent);
	
		machine->networkOverlapped.hEvent = WSACreateEvent();
		mxWSAThrowElse(machine->networkOverlapped.hEvent);
		machine->networkBuf.len = mxNetworkBufferSize - 1;
		machine->networkBuf.buf = machine->networkBuffer;
		
		fxCountMachines(self);
		
		fxReadNetwork(machine, fxReadNetworkAux(machine));
	}
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	char configuration[256];
	DCB dcb;
	COMMTIMEOUTS timeouts;
	
	sprintf(configuration, "baud=%d parity=%c data=%d stop=%d", self->baud, self->parity, self->data, self->stop);

  	self->serialConnection = CreateFile(self->path, GENERIC_READ | GENERIC_WRITE, 0, 0,  OPEN_EXISTING, 0, NULL);
	mxThrowElse(self->serialConnection != INVALID_HANDLE_VALUE);
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	BuildCommDCB(configuration, &dcb);
	mxThrowElse(SetCommState(self->serialConnection, &dcb));
	memset(&timeouts, 0, sizeof(timeouts));
	timeouts.ReadIntervalTimeout = 1; 
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	mxThrowElse(SetCommTimeouts(self->serialConnection, &timeouts));
	CloseHandle(self->serialConnection);

  	self->serialConnection = CreateFile(self->path, GENERIC_READ | GENERIC_WRITE, 0, 0,  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  	
// 	mxThrowElse(GetCommState(self->serialConnection, &dcb));
// 	fprintf(stderr, "# BaudRate %d\n", dcb.BaudRate);
// 	fprintf(stderr, "# fBinary %d\n", dcb.fBinary);
// 	fprintf(stderr, "# fParity %d\n", dcb.fParity);
// 	fprintf(stderr, "# fOutxCtsFlow %d\n", dcb.fOutxCtsFlow);
// 	fprintf(stderr, "# fOutxDsrFlow %d\n", dcb.fOutxDsrFlow);
// 	fprintf(stderr, "# fDtrControl %d\n", dcb.fDtrControl);
// 	fprintf(stderr, "# fDsrSensitivity %d\n", dcb.fDsrSensitivity);
// 	fprintf(stderr, "# fTXContinueOnXoff %d\n", dcb.fTXContinueOnXoff);
// 	fprintf(stderr, "# fOutX %d\n", dcb.fOutX);
// 	fprintf(stderr, "# fInX %d\n", dcb.fInX);
// 	fprintf(stderr, "# fErrorChar %d\n", dcb.fErrorChar);
// 	fprintf(stderr, "# fNull %d\n", dcb.fNull);
// 	fprintf(stderr, "# fRtsControl %d\n", dcb.fRtsControl);
// 	fprintf(stderr, "# fAbortOnError %d\n", dcb.fAbortOnError);
// 	fprintf(stderr, "# fDummy2 %d\n", dcb.fDummy2);
// 	fprintf(stderr, "# wReserved %d\n", dcb.wReserved);
// 	fprintf(stderr, "# XonLim %d\n", dcb.XonLim);
// 	fprintf(stderr, "# XoffLim %d\n", dcb.XoffLim);
// 	fprintf(stderr, "# ByteSize %d\n", dcb.ByteSize);
// 	fprintf(stderr, "# Parity %d\n", dcb.Parity);
// 	fprintf(stderr, "# StopBits %d\n", dcb.StopBits);
// 	fprintf(stderr, "# XonChar %d\n", dcb.XonChar);
// 	fprintf(stderr, "# XoffChar %d\n", dcb.XoffChar);
// 	fprintf(stderr, "# ErrorChar %d\n", dcb.ErrorChar);
// 	fprintf(stderr, "# EofChar %d\n", dcb.EofChar);
// 	fprintf(stderr, "# EvtChar %d\n", dcb.EvtChar);
// 	fprintf(stderr, "# wReserved1 %d\n", dcb.wReserved1);
// 	mxThrowElse(GetCommTimeouts(self->serialConnection, &timeouts));
// 	fprintf(stderr, "# ReadIntervalTimeout %d\n", timeouts.ReadIntervalTimeout);
// 	fprintf(stderr, "# ReadTotalTimeoutMultiplier %d\n", timeouts.ReadTotalTimeoutMultiplier);
// 	fprintf(stderr, "# ReadTotalTimeoutConstant %d\n", timeouts.ReadTotalTimeoutConstant);
// 	fprintf(stderr, "# WriteTotalTimeoutMultiplier %d\n", timeouts.WriteTotalTimeoutMultiplier);
// 	fprintf(stderr, "# WriteTotalTimeoutConstant %d\n", timeouts.WriteTotalTimeoutConstant);
   
	self->serialEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	mxThrowElse(self->serialEvent);

	self->serialOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	mxThrowElse(self->serialOverlapped.hEvent);
	fxReadSerial(self, fxReadSerialAux(self));
}

void fxReadNetwork(txSerialMachine machine, DWORD size)
{
	txSerialTool self = machine->tool;
	while (size > 0) {
		char* former = machine->networkBuffer;
		char* current = former;
		char* limit = former + size;
		int offset;
		while (current < limit) {
			offset = current - former;
			if ((offset >= 3) && (current[-3] == 13) && (current[-2] == 10) && (current[-1] == '<')) {
				fxWriteSerial(self, former, offset);
				fxWriteSerial(self, machine->tag, mxTagSize);
				former = current;
			}
			current++;
		}
		offset = limit - former;
		if (offset)
			fxWriteSerial(self, former, offset);
		WSAResetEvent(machine->networkOverlapped.hEvent);
		size = fxReadNetworkAux(machine);
	}
}

DWORD fxReadNetworkAux(txSerialMachine machine)
{
	txSerialTool self = machine->tool;
	DWORD size, flags = 0;
	if (WSARecv(machine->networkConnection, &machine->networkBuf, 1, &size, &flags, &(machine->networkOverlapped), NULL)) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) mxThrowError(error);
		return 0;
	}
	return size;
}

void fxReadSerial(txSerialTool self, DWORD size)
{
	while (size > 0) {
		char* src = self->serialBuffer;
		char* srcLimit = src + size;
		int offset = self->index;
		char* dst = self->buffer + offset;
		char* dstLimit = self->buffer + mxBufferSize;
#if mxTrace
		fprintf(stderr, "%.*s", size, src);
#endif
		while (src < srcLimit) {
			if (dst == dstLimit) {
				fxWriteNetwork(self, self->buffer, mxBufferSize - mxTagSize);
				memmove(self->buffer, dstLimit - mxTagSize, mxTagSize);
				dst = self->buffer + mxTagSize;
				offset = mxTagSize;
			}
			*dst++ = *src++;
			offset++;
			if ((offset >= 2) && (dst[-2] == 13) && (dst[-1] == 10)) {
				BOOL flag;
				uint32_t value;
				if ((offset >= mxTagSize) && fxMatchProcessingInstruction(dst - mxTagSize, &flag, &value)) {
					if (flag)
						self->currentMachine = fxOpenNetwork(self, value);
					else
						fxCloseNetwork(self, value);
				}
				else if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '/') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] == 'b') && (dst[-5] == 'u') && (dst[-4] == 'g') && (dst[-3] == '>')) {
					fxWriteNetwork(self, self->buffer, offset);
				}
				else {
					dst[-2] = 0;	
					if (offset > 2) fprintf(stderr, "%s\n", self->buffer);
				}
				dst = self->buffer;
				offset = 0;
			}
		}
		self->index = offset;
		
		size = fxReadSerialAux(self);
	}
}

DWORD fxReadSerialAux(txSerialTool self)
{
	DWORD size;
	if (!ReadFile(self->serialConnection, self->serialBuffer, mxSerialBufferSize - 1, &size, &self->serialOverlapped)) {
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) mxThrowError(error);
		return 0;
	}
	return size;
}

void fxWriteNetwork(txSerialTool self, char* buffer, DWORD size)
{
	txSerialMachine machine = self->currentMachine;
	if (machine) {
		WSAOVERLAPPED overlapped;
		WSABUF buf;
		DWORD offset, flags;
		WSAResetEvent(machine->networkEvent);
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = machine->networkEvent;
		buf.len = size;
		buf.buf = buffer;
		if (WSASend(machine->networkConnection, &buf, 1, &offset, 0, &overlapped, NULL) == SOCKET_ERROR) {
			DWORD error = WSAGetLastError();
			if (error != WSA_IO_PENDING) mxThrowError(error);
			mxThrowElse(WaitForSingleObject(overlapped.hEvent, INFINITE) == 0);
			mxWSAThrowElse(WSAGetOverlappedResult(machine->networkConnection, &overlapped, &offset, FALSE, &flags));
		}
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, DWORD size)
{
	OVERLAPPED overlapped;
	DWORD offset;
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	ResetEvent(self->serialEvent);
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = self->serialEvent;
	if (!WriteFile(self->serialConnection, buffer, size, &offset, &overlapped)) {
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING) mxThrowError(error);
		mxThrowElse(WaitForSingleObject(overlapped.hEvent, INFINITE) == 0);
        mxThrowElse(GetOverlappedResult(self->serialConnection, &overlapped, &offset, FALSE));
	}
}

static txSerialToolRecord tool;

void fxSignalHandler(int s)
{
	txSerialTool self = &tool;
	SetEvent(self->events[10]);
}

int main(int argc, char* argv[]) 
{
	int result = 0;
	txSerialTool self = &tool;
	char path[256];
	DWORD size, flags;

	if (argc < 4) {
		fprintf(stderr, "### serial2xsbug <port name> <baud rate> <data bits><parity><stop bits>\n");
		return 1;
	}
	memset(&tool, 0, sizeof(tool));
	strcpy(path, "\\\\.\\");
	strcat(path, argv[1]);
	self->path = path;
	self->baud = atoi(argv[2]);
	self->data = argv[3][0] - '0';
	self->parity = argv[3][1];
	self->stop = argv[3][2] - '0';
	self->serialConnection = INVALID_HANDLE_VALUE;

	self->host = "localhost";
	self->port = 5002;
	
	if (setjmp(self->_jmp_buf) == 0) {
		WSADATA wsaData;
		self->error = WSAStartup(0x202, &wsaData);
		self->signalEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		signal(SIGINT, &fxSignalHandler);
		signal(SIGBREAK, &fxSignalHandler);
		fxOpenSerial(self);
		self->count = 2;
		self->events[0] = self->signalEvent;
		self->events[1] = self->serialOverlapped.hEvent;
		for (;;) {
			DWORD which = WaitForMultipleObjects(self->count, self->events, FALSE, INFINITE);
			if (which == 0)
				break;
			if (which == 1) {
				mxThrowElse(GetOverlappedResult(self->serialConnection, &self->serialOverlapped, &size, FALSE))
				fxReadSerial(self, size);
			}
			else if (which < self->count) {
				txSerialMachine machine = self->firstMachine;
				which -= 2;
				while (which) {
					machine = machine->nextMachine;
					which--;
				}
				mxThrowElse(WSAGetOverlappedResult(machine->networkConnection, &machine->networkOverlapped, &size, FALSE, &flags));
				fxReadNetwork(machine, size);
			}
		}
	}
	else {
		char buffer[2048];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, self->error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
		if (self->file)
			fprintf(stderr, "%s(%d): %s\n", self->file, self->line, buffer);
		else
			fprintf(stderr, "# %s\n", buffer);
		result = 1;
	}
	fxCloseSerial(self);
	return result;
}
