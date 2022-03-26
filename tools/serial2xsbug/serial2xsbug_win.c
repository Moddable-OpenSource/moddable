/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "serial2xsbug.h"

static void fxCountMachines(txSerialTool self);
static void fxProgrammingModeSerial(txSerialTool self);
static void fxReadNetwork(txSerialMachine machine, DWORD size);
static DWORD fxReadNetworkAux(txSerialMachine machine);
static void fxReadSerial(txSerialTool self, DWORD size);
static DWORD fxReadSerialAux(txSerialTool self);

#define mxThrowError(_ERROR) (self->file=__FILE__, self->line=__LINE__, self->error=_ERROR, longjmp(self->_jmp_buf, 1))
#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = GetLastError(); longjmp(self->_jmp_buf, 1); } }
#define mxWSAThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = WSAGetLastError(); longjmp(self->_jmp_buf, 1); } }

void fxCloseNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if ((machine->value == value) || (0 == value)) {
			*link = machine->nextMachine;
			if (self->currentMachine == machine)
				self->currentMachine = NULL;
			CloseHandle(machine->networkOverlapped.hEvent);
			CloseHandle(machine->networkEvent);
			if (machine->networkConnection != INVALID_SOCKET)
				closesocket(machine->networkConnection);
			free(machine);
		}
		else
			link = &(machine->nextMachine);
	}
	fxCountMachines(self);
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
	machine->receiveCount += 1;
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	char configuration[256];
	DCB dcb;
	COMMTIMEOUTS timeouts;
	
	sprintf(configuration, "baud=%d parity=%c data=%d stop=%d", self->baud, self->parity, self->data, self->stop);

  	self->serialConnection = CreateFile(self->path, GENERIC_READ | GENERIC_WRITE, 0, 0,  OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == self->serialConnection) {
		if (!self->reconnecting)
			mxThrowElse(self->serialConnection != INVALID_HANDLE_VALUE);
		return;
	}
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
   
	if (NULL == self->serialEvent) {
		self->serialEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		mxThrowElse(self->serialEvent);
	}

	if (NULL == self->serialOverlapped.hEvent) {
		self->serialOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		mxThrowElse(self->serialOverlapped.hEvent);
	}

	fxReadSerial(self, fxReadSerialAux(self));
	
	if (self->programming) {
#if mxTraceCommands
		fprintf(stderr, "### programming mode\n");
#endif
		fxProgrammingModeSerial(self);
		exit(0);
	}

	if (self->restartOnConnect) {
		self->restartOnConnect = 0;
		fxRestart(self);
	}
}

void fxProgrammingModeSerial(txSerialTool self)
{
	mxThrowElse(EscapeCommFunction(self->serialConnection, SETRTS) != 0);
	mxThrowElse(EscapeCommFunction(self->serialConnection, SETDTR) != 0);

	Sleep(10);

	mxThrowElse(EscapeCommFunction(self->serialConnection, CLRDTR) != 0);	
	Sleep(100);

	mxThrowElse(EscapeCommFunction(self->serialConnection, CLRRTS) != 0);
	mxThrowElse(EscapeCommFunction(self->serialConnection, SETDTR) != 0);
	
	Sleep(50);

	mxThrowElse(EscapeCommFunction(self->serialConnection, CLRDTR) != 0);
}

void fxReadNetwork(txSerialMachine machine, DWORD size)
{
	while (size > 0) {
		fxReadNetworkBuffer(machine, machine->networkBuffer, (int)size);
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
		fxReadSerialBuffer(self, self->serialBuffer, (int)size);
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

void fxRestartSerial(txSerialTool self)
{
	mxThrowElse(EscapeCommFunction(self->serialConnection, SETRTS) != 0);
	mxThrowElse(EscapeCommFunction(self->serialConnection, CLRDTR) != 0);
	Sleep(5);
	mxThrowElse(EscapeCommFunction(self->serialConnection, CLRRTS) != 0);
	if (self->dtr)
		mxThrowElse(EscapeCommFunction(self->serialConnection, SETDTR) != 0);
}

void fxWriteNetwork(txSerialMachine machine, char* buffer, int size)
{
	txSerialTool self = machine->tool;
	WSAOVERLAPPED overlapped;
	WSABUF buf;
	DWORD offset, flags;
	WSAResetEvent(machine->networkEvent);
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = machine->networkEvent;
	buf.len = (DWORD)size;
	buf.buf = buffer;
	if (WSASend(machine->networkConnection, &buf, 1, &offset, 0, &overlapped, NULL) == SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) mxThrowError(error);
		mxThrowElse(WaitForSingleObject(overlapped.hEvent, INFINITE) == 0);
		mxWSAThrowElse(WSAGetOverlappedResult(machine->networkConnection, &overlapped, &offset, FALSE, &flags));
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, int size)
{
	OVERLAPPED overlapped;
	DWORD offset;
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	ResetEvent(self->serialEvent);
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = self->serialEvent;
	if (!WriteFile(self->serialConnection, buffer, (DWORD)size, &offset, &overlapped)) {
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING) mxThrowError(error);
		mxThrowElse(WaitForSingleObject(overlapped.hEvent, INFINITE) == 0);
        mxThrowElse(GetOverlappedResult(self->serialConnection, &overlapped, &offset, FALSE));
	}
}

static txSerialToolRecord tool;

static void fxSignalHandler(int s)
{
	txSerialTool self = &tool;
	SetEvent(self->events[0]);
}

int main(int argc, char* argv[]) 
{
	txSerialTool self = &tool;
	char path[256];
	DWORD size, flags;
	int result = fxArguments(self, argc, argv);
	if (result)
		return result;
	strcpy(path, "\\\\.\\");
	strcat(path, self->path);
	self->path = path;
	self->serialConnection = INVALID_HANDLE_VALUE;
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
			if (self->reconnecting) {
				fxOpenSerial(self);
				if (INVALID_HANDLE_VALUE == self->serialConnection)
					Sleep(500);
				else {
					self->reconnecting = 0;
					self->events[0] = self->signalEvent;
					self->events[1] = self->serialOverlapped.hEvent;
				}
				continue;
			}
			DWORD which = WaitForMultipleObjects(self->count, self->events, FALSE, INFINITE);
			if (which == 0) {
				result = 1;
				break;
			}
			if (which == 1) {
				if (!GetOverlappedResult(self->serialConnection, &self->serialOverlapped, &size, FALSE)) {
					fxCloseSerial(self);
					self->reconnecting = 1;
					Sleep(500);
					continue;
				}
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
