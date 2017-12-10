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

#include <netdb.h>
#include <sys/ioctl.h>

static void fxQueuePromiseJobsCallback(void *info);

void fxCreateMachinePlatform(txMachine* the)
{
	CFRunLoopSourceContext sourceContext;
	memset(&sourceContext, 0, sizeof(CFRunLoopSourceContext));
	sourceContext.info = (void*)the;
	sourceContext.perform = &fxQueuePromiseJobsCallback;
	the->promiseSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), the->promiseSource, kCFRunLoopCommonModes);
}

void fxDeleteMachinePlatform(txMachine* the)
{
	if (the->promiseSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), the->promiseSource, kCFRunLoopCommonModes);
		CFRelease(the->promiseSource);
	}
}

void fxQueuePromiseJobs(txMachine* the)
{
	CFRunLoopSourceSignal(the->promiseSource);
}

void fxQueuePromiseJobsCallback(void *info)
{
	txMachine* the = info;
	fxRunPromiseJobs(the);
}

#ifdef mxDebug
void fxReadableCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	txMachine* the = context;
	if (fxIsReadable(the)) {
		fxDebugCommand(the);
		if (the->breakOnStartFlag) {
			fxBeginHost(the);
			fxDebugLoop(the, NULL, 0, "C: xsDebugger");
			fxEndHost(the);
		}
	}
}

void fxConnect(txMachine* the)
{
	CFSocketContext context;
	struct hostent *host;
	struct sockaddr_in address;
	CFDataRef data = NULL;
	host = gethostbyname("localhost");
	if (!host)
		return;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	memcpy(&(address.sin_addr), host->h_addr, host->h_length);
	CFStringRef identifier = CFBundleGetIdentifier(CFBundleGetMainBundle());
	if (identifier && CFStringHasSuffix(identifier, CFSTR(".xsbug")))
		address.sin_port = htons(5003);
	else
		address.sin_port = htons(5002);
	c_memset(&context, 0, sizeof(CFSocketContext));
	context.info = (void*)the;
	the->connection = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketReadCallBack, fxReadableCallback, &context);
	data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)&address, sizeof(address));
	if (data) {
		if (CFSocketConnectToAddress(the->connection, data, (CFTimeInterval)2) == 0)  {
			the->connectionSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, the->connection, 0);
			CFRunLoopAddSource(CFRunLoopGetCurrent(), the->connectionSource, kCFRunLoopCommonModes);
		}
		else {
			CFSocketInvalidate(the->connection);
			CFRelease(the->connection);
			the->connection = NULL;
		}
		CFRelease(data);
	}
}

void fxDisconnect(txMachine* the)
{
	CFSocketRef socket = the->connection;
	CFRunLoopSourceRef source = the->connectionSource;
	if (source) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
		CFRelease(source);
		the->connectionSource = NULL;
	}
	if (socket) {
		CFSocketInvalidate(socket);
		CFRelease(socket);
		the->connection = NULL;
	}
}

txBoolean fxIsConnected(txMachine* the)
{
	return (the->connection) ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	int count;
	return the->connection && (ioctl(CFSocketGetNative(the->connection), FIONREAD, &count) == 0) && (count > 0);
}

void fxReceive(txMachine* the)
{
	if (the->connection) {
		int count;
	again:
		count = read(CFSocketGetNative(the->connection), the->debugBuffer, sizeof(the->debugBuffer) - 1);
		if (count < 0) {
			if (errno == EINTR)
				goto again;
			fxDisconnect(the);
		}
		the->debugOffset = count;
		the->debugBuffer[the->debugOffset] = 0;
	}
}

void fxSend(txMachine* the, txBoolean more)
{
	if (the->connection) {
	again:
		if (write(CFSocketGetNative(the->connection), the->echoBuffer, the->echoOffset) <= 0) {
			if (errno == EINTR)
				goto again;
			fxDisconnect(the);
		}
	}
}
#endif
