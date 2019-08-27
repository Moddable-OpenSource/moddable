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
#include "xsHost.h"

#include "qapi_netservices.h"
#include "qurt_error.h"
#include "qurt_thread.h"

#define RESOLVER_THREAD_PRIORITY	19
#define RESOLVER_THREAD_STACK_SIZE	2048

typedef struct {
	xsSlot			callback;
	xsMachine		*the;
	qurt_thread_t	thread;
	char			ip[20];
	uint8_t			resolved;
	char			name[1];
} xsNetResolveRecord, *xsNetResolve;

static void resolver_task(void *pvParameter);
static void resolved(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_net_resolve(xsMachine *the)
{
	xsNetResolve nr;
	char *name = xsToString(xsArg(0));
	int nameLen = c_strlen(name);
	qurt_thread_attr_t	attr;
	uint8_t result;

	if (0 == qapi_Net_DNSc_Is_Started()) {
		qapi_Net_DNSc_Command(QAPI_NET_DNS_START_E);
		if (0 == qapi_Net_DNSc_Is_Started())
			xsUnknownError("dns initialization failed");
	}
	
	nr = c_calloc(sizeof(xsNetResolveRecord) + nameLen, 1);
	if (!nr)
		xsUnknownError("out of memory");

	nr->the = the;
	nr->callback = xsArg(1);
	xsRemember(nr->callback);
	xsToStringBuffer(xsArg(0), nr->name, nameLen + 1);

	qurt_thread_attr_init(&attr);
	qurt_thread_attr_set_name(&attr, "dnsc");
	qurt_thread_attr_set_priority(&attr, RESOLVER_THREAD_PRIORITY);
	qurt_thread_attr_set_stack_size(&attr, RESOLVER_THREAD_STACK_SIZE);
	result = qurt_thread_create(&nr->thread, &attr, resolver_task, nr);
	if (QURT_EOK != result) {
		xsForget(nr->callback);
		c_free(nr);
		xsUnknownError("dns initialization failed");
	}
}

void resolver_task(void *pvParameter)
{
	xsNetResolve nr = (xsNetResolve)pvParameter;
	struct qapi_hostent_s *host;

	host = gethostbyname(nr->name);
	if (host && (host->h_addrtype == AF_INET)) {
		struct in_addr addr;
		addr.s_addr = *(u_long *)host->h_addr_list[0];
		inet_ntop(AF_INET, &addr, nr->ip, 20);
		nr->resolved = (c_strlen(nr->ip) > 0);
	}
	modMessagePostToMachine(nr->the, NULL, 0, resolved, nr);
    qurt_thread_stop();
}

void resolved(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsNetResolve nr = refcon;

	xsBeginHost(nr->the);

	if (nr->resolved)
		xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsString(nr->ip));
	else
		xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsUndefined);

	xsEndHost(the);

	xsForget(nr->callback);
	c_free(nr);
}
