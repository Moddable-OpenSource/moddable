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
#include "xsHost.h"

#include "malloc.h"
#include "string.h"

#include "lwip/tcp.h"

typedef struct xsNetResolveRecord xsNetResolveRecord;
typedef xsNetResolveRecord *xsNetResolve;

struct xsNetResolveRecord {
	xsNetResolve	next;
	xsSlot			callback;
	xsMachine		*the;
	ip_addr_t		ipaddr;
	uint8_t			started;
	uint8_t			resolved;
	char			name[1];
};

static void didResolve(const char *name, ip_addr_t *ipaddr, void *arg);
static void resolvedImmediate(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void resolveNext(void);

static xsNetResolve gResolve;

void xs_net_resolve(xsMachine *the)
{
	xsNetResolve nr;
	char *name = xsToString(xsArg(0));
	int nameLen = c_strlen(name);

	nr = c_malloc(sizeof(xsNetResolveRecord) + nameLen);
	if (!nr)
		xsUnknownError("out of memory");

	c_memcpy(nr->name, name, nameLen + 1);
	nr->next = NULL;
	nr->the = the;
	nr->callback = xsArg(1);
	xsRemember(nr->callback);
	nr->started = 0;
	nr->resolved = 0;

	modCriticalSectionBegin();

	if (NULL == gResolve)
		gResolve = nr;
	else {
		xsNetResolve walker = gResolve;
		while (walker->next)
			walker = walker->next;
		walker->next = nr;
	}

	modCriticalSectionEnd();

	resolveNext();
}

void resolveNext(void)
{
	xsNetResolve nr;
	err_t err;

	modCriticalSectionBegin();
		nr = gResolve;
		if (nr) {
			if (nr->started)
				nr = NULL;
			else
				nr->started = 1;
		}
	modCriticalSectionEnd();
	if (!nr) return;

	err = dns_gethostbyname(nr->name, &nr->ipaddr, didResolve, nr);
	if (ERR_OK == err) {
		nr->resolved = 1;
		modMessagePostToMachine(nr->the, NULL, 0, resolvedImmediate, nr);
	}
	else if (ERR_INPROGRESS == err)
		;
	else
		modMessagePostToMachine(nr->the, NULL, 0, resolvedImmediate, nr);
}

void didResolve(const char *name, ip_addr_t *ipaddr, void *arg)
{
	xsNetResolve nr = arg;
	if (ipaddr) {
		nr->ipaddr = *ipaddr;
		nr->resolved = 1;
	}
	modMessagePostToMachine(nr->the, NULL, 0, resolvedImmediate, nr);
}

void resolvedImmediate(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsNetResolve nr = refcon;

	xsBeginHost(nr->the);

	if (nr->resolved) {
		char ip[40];

		ipaddr_ntoa_r(&nr->ipaddr, ip, sizeof(ip));
		xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsString(ip));
	}
	else
		xsCallFunction2(nr->callback, xsGlobal, xsString(nr->name), xsUndefined);

	xsEndHost(the);

	modCriticalSectionBegin();
		gResolve = nr->next;
	modCriticalSectionEnd();

	xsForget(nr->callback);
	c_free(nr);

	resolveNext();
}
