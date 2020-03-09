/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	GResolver		*resolver;
	char			ip[20];
	uint8_t			resolved;
	char			name[1];
} xsNetResolveRecord, *xsNetResolve;

static void resolved(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void resolverCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);

void xs_net_resolve(xsMachine *the)
{
	GResolver *resolver;
	xsNetResolve nr;
	char *name = xsToString(xsArg(0));
	int nameLen = c_strlen(name);

	resolver = g_resolver_get_default();
	if (NULL == resolver)
		xsUnknownError("no resolver");
		
	nr = c_calloc(sizeof(xsNetResolveRecord) + nameLen, 1);
	if (!nr)
		xsUnknownError("out of memory");

	nr->the = the;
	nr->callback = xsArg(1);
	nr->resolver = resolver;
	xsRemember(nr->callback);
	xsToStringBuffer(xsArg(0), nr->name, nameLen + 1);

	g_resolver_lookup_by_name_async(resolver, nr->name, NULL, resolverCallback, nr);
}

void resolverCallback(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
	xsNetResolve nr = (xsNetResolve)user_data;
	GList *addresses;

	addresses = g_resolver_lookup_by_name_finish(nr->resolver, result, NULL);
	if (NULL != addresses) {
		GInetAddress *address = (GInetAddress*)addresses->data;
		if (NULL != address) {
			char *ip = g_inet_address_to_string(address);
			if (NULL != ip &&
			    g_inet_address_get_family(address) == G_SOCKET_FAMILY_IPV4) {
				nr->resolved = 1;
				c_strcpy(nr->ip, ip);
				g_free(ip);
			}
		}
		g_resolver_free_addresses(addresses);
	}
	resolved(nr->the, nr, NULL, 0);
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
	g_object_unref(nr->resolver);
	c_free(nr);
}

