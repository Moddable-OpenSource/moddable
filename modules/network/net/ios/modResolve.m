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

#include "xs.h"

#include <CFNetwork/CFNetwork.h>

typedef struct {
	xsSlot			callback;
	xsMachine		*the;
	char			name[1];
} xsNetResolveRecord, *xsNetResolve;

static void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info);

void xs_net_resolve(xsMachine *the)
{
	xsNetResolve nr;
	char *name = xsToString(xsArg(0));
	int nameLen = (int)c_strlen(name);
	CFStringRef host;

	nr = c_malloc(sizeof(xsNetResolveRecord) + nameLen);
	if (!nr)
		xsUnknownError("out of memory");

	nr->the = the;
	nr->callback = xsArg(1);
	xsRemember(nr->callback);

	xsToStringBuffer(xsArg(0), nr->name, nameLen + 1);

	CFHostRef cfHost;

	host = CFStringCreateWithCString(NULL, (const char *)nr->name, kCFStringEncodingUTF8);
	cfHost = CFHostCreateWithName(kCFAllocatorDefault, host);
	CFRelease(host);

	CFHostClientContext context = {0};
	context.info = nr;

	CFHostSetClient(cfHost, resolved, &context);
	CFHostScheduleWithRunLoop(cfHost, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	// resolve
	CFStreamError streamErr;
	if (!CFHostStartInfoResolution(cfHost, kCFHostAddresses, &streamErr))
		xsUnknownError("cannot resolve host address");
}

void resolved(CFHostRef cfHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info)
{
	xsNetResolve nr = info;
	xsMachine *the = nr->the;

	CFArrayRef cfArray = CFHostGetAddressing(cfHost, NULL);
	xsBeginHost(the);
	if (cfArray) {
		NSData *address = CFArrayGetValueAtIndex(cfArray, CFArrayGetCount(cfArray) - 1);

		const UInt8 *bytes = CFDataGetBytePtr((__bridge CFDataRef)address);
		//@@	CFIndex length = CFDataGetLength((__bridge CFDataRef)address);
		//@@	if (length != sizeof(struct sockaddr_in))
		//@@		xsUnknownError("unexpected sockaddr");

		char ip[20];

		snprintf(ip, sizeof(ip), "%d.%d.%d.%d", (int)bytes[4], (int)bytes[5], (int)bytes[6], (int)bytes[7]);	// almost surely not really correct
		xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsString(ip));
	}
	else
		xsCallFunction1(nr->callback, xsGlobal, xsString(nr->name));
	xsEndHost(the);

	xsForget(nr->callback);
	c_free(nr);
}

