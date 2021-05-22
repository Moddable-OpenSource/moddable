/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

/*
	to do:

		string;ascii doesn't ensure data is valid

*/

#include "xsAll.h"
#include "mc.xs.h"
#include <process.h>

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				onError;
	xsSlot				onReadable;
	xsSlot				onWritable;
	HANDLE				comm;
	HANDLE				commEvent;
	HANDLE				thread;
	CRITICAL_SECTION	readCritical;
	HANDLE				readEvent;
	void*				readJob;
	CRITICAL_SECTION	writeCritical;
	HANDLE				writeEvent;
	void*				writeJob;
	uint8_t				bufferFormat;
	uint8_t				hasOnError;
	uint8_t				hasOnReadable;
	uint8_t				hasOnWritable;
} xsSerialRecord, *xsSerial;

typedef struct {
	txWorkerJob* next;
	txWorkerCallback callback;
	xsSerial serial;
} xsSerialJobRecord, *xsSerialJob;

static void xs_serial_read_callback(void* machine, void* it);
static void xs_serial_throw(xsMachine* the, DWORD error, xsStringValue path, xsIntegerValue line);
static void xs_serial_write_aux(void* machine, xsSerial s);
static void xs_serial_write_callback(void* machine, void* it);
static void xs_serial_format_set_aux(xsMachine *the, xsSerial s, char *format);

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (xs_serial_throw(the,GetLastError(),(char *)__FILE__,__LINE__), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (xs_serial_throw(the,GetLastError(),NULL,0), 1)))
#endif

static unsigned int __stdcall xs_serial_loop(void* it)
{
	xsSerial s = it;
// 	xs_serial_write_aux(the, s);
	for (;;) {
		DWORD which;
		OVERLAPPED overlapped;
 		DWORD count = 0;
 		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = s->commEvent;
  		if (!WaitCommEvent(s->comm, &which, &overlapped)) {
        	DWORD error = GetLastError();
        	if (error == ERROR_IO_PENDING) {
       			if (!GetOverlappedResult(s->comm, &overlapped, &count, TRUE))
        			return 1;
        	}
        	else
				break;
   		}
		EnterCriticalSection(&s->readCritical);
   		if (s->hasOnReadable && (which & EV_RXCHAR) && !s->readJob) {
// 			OutputDebugString("EV_RXCHAR\n");
			xsSerialJob readJob = c_calloc(sizeof(xsSerialJobRecord), 1);
			if (readJob == NULL)
				break;
			readJob->callback = xs_serial_read_callback;
			readJob->serial = s;
			fxQueueWorkerJob(s->the, readJob);
			s->readJob = readJob;
		}
		LeaveCriticalSection(&s->readCritical);
// 		if (which & EV_TXEMPTY) {
// 			OutputDebugString("EV_TXEMPTY\n");
// 			xs_serial_write_aux(the, s);
// 		}
	}
	return 0;
}

void xs_serial_destructor(void *data)
{
	xsSerial s = data;
	if (!s) return;
	if (s->comm != INVALID_HANDLE_VALUE) {
    	PurgeComm(s->comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   		CancelIoEx(s->comm, NULL);
		CloseHandle(s->comm);
	}
	if (s->thread != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(s->thread, INFINITE);
		CloseHandle(s->thread);
	}
	if (s->commEvent != INVALID_HANDLE_VALUE)
		CloseHandle(s->commEvent);
	if (s->readEvent != INVALID_HANDLE_VALUE)
		CloseHandle(s->readEvent);
	if (s->readJob)
		((xsSerialJob)s->readJob)->serial = NULL;
	DeleteCriticalSection(&s->readCritical);
	if (s->writeEvent != INVALID_HANDLE_VALUE)
		CloseHandle(s->writeEvent);
	if (s->writeJob)
		((xsSerialJob)s->writeJob)->serial = NULL;
	DeleteCriticalSection(&s->writeCritical);
	free(data);
}

void xs_serial_constructor(xsMachine *the)
{
	xsSerial s = NULL;
	xsIntegerValue baud;
	xsStringValue device;
 	char configuration[256];
	DCB dcb;
	COMMTIMEOUTS timeouts;
	xsVars(1);
	xsTry {
		s = calloc(1, sizeof(xsSerialRecord));
		xsElseThrow(s != NULL);
		InitializeCriticalSection(&s->readCritical);
		InitializeCriticalSection(&s->writeCritical);
		s->the = the;
		s->obj = xsThis;
		s->onError = xsGet(xsArg(0), xsID_onError);
		s->hasOnError = xsTest(s->onError);
		s->onReadable = xsGet(xsArg(0), xsID_onReadable);
		s->hasOnReadable = xsTest(s->onReadable);
		s->onWritable = xsGet(xsArg(0), xsID_onWritable);
		s->hasOnWritable = xsTest(s->onWritable);
		if (xsHas(xsArg(0), xsID_format)) {
			xsVar(0) = xsGet(xsArg(0), xsID_format);
			xs_serial_format_set_aux(the, s, xsToString(xsVar(0)));
		}
		else
			s->bufferFormat = 1;
		if (xsHas(xsArg(0), xsID_target)) {
			xsVar(0) = xsGet(xsArg(0), xsID_target);
			xsSet(xsThis, xsID_target, xsVar(0));
		}
		xsVar(0) = xsGet(xsArg(0), xsID_baud);
		baud = xsToInteger(xsVar(0));
		xsVar(0) = xsGet(xsArg(0), xsID_device);
		device = xsToString(xsVar(0));

		s->comm = CreateFile(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		xsElseThrow(s->comm != INVALID_HANDLE_VALUE);
		sprintf(configuration, "baud=%d parity=N data=8 stop=1", baud);
		memset(&dcb, 0, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		BuildCommDCB(configuration, &dcb);
		xsElseThrow(SetCommState(s->comm, &dcb));
		memset(&timeouts, 0, sizeof(timeouts));
		timeouts.ReadIntervalTimeout = 0; 
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;
		xsElseThrow(SetCommTimeouts(s->comm, &timeouts));
		CloseHandle(s->comm);
		s->comm = INVALID_HANDLE_VALUE;
	
		s->comm = CreateFile(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
		xsElseThrow(SetCommMask(s->comm, EV_RXCHAR /* | EV_TXEMPTY */));
		s->commEvent = CreateEvent(0, TRUE, FALSE, NULL);
		xsElseThrow(s->commEvent != INVALID_HANDLE_VALUE);
		s->readEvent = CreateEvent(0, TRUE, FALSE, NULL);
		xsElseThrow(s->readEvent != INVALID_HANDLE_VALUE);
		s->writeEvent = CreateEvent(0, TRUE, FALSE, NULL);
		xsElseThrow(s->writeEvent != INVALID_HANDLE_VALUE);
		s->thread = (HANDLE)_beginthreadex(NULL, 0, xs_serial_loop, s, 0, NULL);
		xsElseThrow(s->thread != INVALID_HANDLE_VALUE);
	}
	xsCatch {
		xs_serial_destructor(s);
		xsThrow(xsException);
	}	
	xsSetHostData(xsThis, s);
	xsRemember(s->obj);
	if (s->hasOnError)
		xsRemember(s->onError);
	if (s->hasOnReadable)
		xsRemember(s->onReadable);
	if (s->hasOnWritable)
		xsRemember(s->onWritable);
	xs_serial_write_aux(the, s);
}

void xs_serial_check(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	DCB dcb;
	if (GetCommState(s->comm, &dcb) && SetCommState(s->comm, &dcb))
		return;
	if (s->hasOnError) {
		xsTry {
			xsCallFunction0(s->onError, s->obj);
		}
		xsCatch {
		}
	}
	xsCall0(s->obj, xsID_close);
}

void xs_serial_close(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;

	xsForget(s->obj);
	if (s->hasOnError)
		xsForget(s->onError);
	if (s->hasOnReadable)
		xsForget(s->onReadable);
	if (s->hasOnWritable)
		xsForget(s->onWritable);
	xs_serial_destructor(s);
	
	xsSetHostData(xsThis, NULL);
}

void xs_serial_read(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	DWORD errors;
	COMSTAT	comstat;
	DWORD available;
	void *data;
    OVERLAPPED overlapped;
	DWORD read;
	DWORD read2 = 0;
	
	xsElseThrow(ClearCommError(s->comm, &errors, &comstat));
	available = comstat.cbInQue;		
	if (!available)
		return;		
	if (s->bufferFormat) {
		xsResult = xsArrayBuffer(NULL, (xsIntegerValue)available);
		data = xsToArrayBuffer(xsResult);
	}
	else {
		xsResult = xsStringBuffer(NULL, (xsIntegerValue)available);
		data = xsToString(xsResult);
	}
 	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = s->readEvent;
	if (!ReadFile(s->comm, data, available, &read, &overlapped)) {
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING) xs_serial_throw(the, error, NULL, 0);
        xsElseThrow(GetOverlappedResult(s->comm, &overlapped, &read2, TRUE));
	}
// 	{
// 		char buffer[256];
// 		sprintf(buffer, "READ %ld %ld %ld\n", available, read, read2);
// 		OutputDebugString(buffer);
// 	}
}

void xs_serial_read_callback(void* machine, void* it)
{
	xsSerialJob readJob = it;
	xsSerial s = readJob->serial;
	if (s) {
		EnterCriticalSection(&s->readCritical);
		s->readJob = NULL;
		LeaveCriticalSection(&s->readCritical);
		xsBeginHost(machine);
		xsTry {
			DWORD errors;
			COMSTAT	comstat;
			xsElseThrow(ClearCommError(s->comm, &errors, &comstat));
			xsCallFunction1(s->onReadable, s->obj, xsInteger((xsIntegerValue)comstat.cbInQue));
		}
		xsCatch {
		}
		xsEndHost(machine);
	}
}

void xs_serial_purge(xsMachine *the)
{
}

void xs_serial_set(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);

	if (xsHas(xsArg(0), xsID_DTR)) {
		xsResult = xsGet(xsArg(0), xsID_DTR);
		if (xsTest(xsResult))
			xsElseThrow(EscapeCommFunction(s->comm, SETDTR));
		else
			xsElseThrow(EscapeCommFunction(s->comm, CLRDTR));
	}

	if (xsHas(xsArg(0), xsID_RTS)) {
		xsResult = xsGet(xsArg(0), xsID_RTS);
		if (xsTest(xsResult))
			xsElseThrow(EscapeCommFunction(s->comm, SETRTS));
		else
			xsElseThrow(EscapeCommFunction(s->comm, CLRRTS));
	}
}

void xs_serial_throw(xsMachine* the, DWORD error, xsStringValue path, xsIntegerValue line)
{
	char buffer[2048];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
	fxThrowMessage(the, path, line, XS_UNKNOWN_ERROR, "%s", buffer);
}

void xs_serial_write(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	char *data;
	DWORD count;
    OVERLAPPED overlapped;
	DWORD written;
	DWORD written2 = 0;

	if (s->bufferFormat) {
		data = xsToArrayBuffer(xsArg(0));
		count = (DWORD)xsGetArrayBufferLength(xsArg(0));
	}
	else {
		data = xsToString(xsArg(0));
		count = (DWORD)strlen(data);
	}
	if (!count)
		return;
 	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = s->writeEvent;
	if (!WriteFile(s->comm, data, (DWORD)count, &written, &overlapped)) {
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING) xs_serial_throw(the, error, NULL, 0);
        xsElseThrow(GetOverlappedResult(s->comm, &overlapped, &written2, TRUE));
	}
// 	{
// 		char buffer[256];
// 		sprintf(buffer, "WRITE %ld %ld %ld\n", count, written, written2);
// 		OutputDebugString(buffer);
// 	}
	xs_serial_write_aux(the, s);
}

void xs_serial_write_aux(void* machine, xsSerial s)
{
	if (s->hasOnWritable) {
// 		EnterCriticalSection(&s->writeCritical);
		if (!s->writeJob) {
			xsSerialJob writeJob = c_calloc(sizeof(xsSerialJobRecord), 1);
			if (writeJob == NULL)
				return;
			writeJob->callback = xs_serial_write_callback;
			writeJob->serial = s;
			fxQueueWorkerJob(s->the, writeJob);
			s->writeJob = writeJob;
		}
//  		LeaveCriticalSection(&s->writeCritical);
	}
}

void xs_serial_write_callback(void* machine, void* it)
{
	xsSerialJob writeJob = it;
	xsSerial s = writeJob->serial;
	if (s) {
//  		EnterCriticalSection(&s->writeCritical);
		s->writeJob = NULL;
//  		LeaveCriticalSection(&s->writeCritical);
		xsBeginHost(machine);
		xsTry {
			xsCallFunction1(s->onWritable, s->obj, xsInteger(1024));
		}
		xsCatch {
		}
		xsEndHost(machine);
	}
}

void xs_serial_format_get(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;
	if (s->bufferFormat)
		xsResult = xsString("buffer");
	else
		xsResult = xsString("string;ascii");
}

void xs_serial_format_set(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;
	xs_serial_format_set_aux(the, s, xsToString(xsArg(0)));
}

void xs_serial_format_set_aux(xsMachine *the, xsSerial s, char *format)
{
	if (!strcmp(format, "buffer"))
		s->bufferFormat = 1;
	else if (!strcmp(format, "string;ascii"))
		s->bufferFormat = 0;
	else
		xsUnknownError("invalid format");
}
