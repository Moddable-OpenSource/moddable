/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

static void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

typedef struct {
	xsSlot				obj;
	xsMachine			*the;
	struct net_if		*iface;

	atomic_t				useCount;
	uint8_t				connecting:1;
	uint8_t				connected:1;
	uint8_t				ipAddress:1;
	uint8_t				closed:1;

	xsSlot				*onChanged;

	struct net_mgmt_event_callback callbackIPv4;
} xsEthernetRecord, *xsEthernet;

void xs_ethernet_destructor(void *data)
{
	xsEthernet eth = (xsEthernet)data;
	if (!eth) return;

	if (atomic_dec(&eth->useCount) > 0)
		return;

	c_free(eth);
}

static void xs_ethernet_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	xsEthernet eth = (xsEthernet)it;

	if (eth->onChanged)
		(*markRoot)(the, eth->onChanged);
}

static const xsHostHooks xsEthernetHooks = {
	xs_ethernet_destructor,
	xs_ethernet_mark,
	C_NULL
};

void xs_ethernet(xsMachine *the)
{
//@@ port
	struct net_if *iface;

	for (int i = 1; (iface = net_if_get_by_index(i)) != C_NULL; i++) {
		if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET))
			break;
	}

	if (C_NULL == iface)
		xsUnknownError("no ethernet");
	
	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);

	xsEthernet eth = c_calloc(1, sizeof(xsEthernetRecord));
	eth->the = the;
	eth->obj = xsThis;
	atomic_set(&eth->useCount, 1);
	eth->iface = iface;
	eth->onChanged = onChanged;
	eth->connected = net_if_oper_state(iface) == NET_IF_OPER_UP;
	eth->ipAddress = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)? 1 : 0;

	xsmcSetHostData(xsThis, eth);
	xsSetHostHooks(xsThis, &xsEthernetHooks);

	xsRemember(eth->obj);

	net_mgmt_init_event_callback(&eth->callbackIPv4, ipv4_event_handler, NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&eth->callbackIPv4);
}

void xs_ethernet_close(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostData(xsThis);
	if (!eth) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	eth->closed = 1;
	xsForget(eth->obj);
	net_mgmt_del_event_callback(&eth->callbackIPv4);
	xs_ethernet_destructor(eth);
	xsmcSetHostData(xsThis, C_NULL);
	xsSetHostDestructor(xsThis, C_NULL);
}

static void ethernetConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsEthernet eth = refcon;
	uint64_t mgmt_event = *(uint64_t *)msgIn;
	uint8_t connecting = eth->connecting, connected = eth->connected, ipAddress = eth->ipAddress;

	if (eth->closed)
		goto bail;

	xsBeginHost(the);
		if (NET_EVENT_IPV4_ADDR_DEL == mgmt_event) {
			eth->connecting = 0;
			eth->connected = 0;
			eth->ipAddress = 0;
		}
		else if (NET_EVENT_IPV4_ADDR_ADD == mgmt_event) {
			eth->connecting = 0;
			eth->connected = 1;
			eth->ipAddress = 1;
		}

		if (eth->onChanged &&
			((connecting != eth->connecting) ||
			(connected != eth->connected) ||
			(ipAddress != eth->ipAddress)))
			xsCallFunction0(xsReference(eth->onChanged), eth->obj);
	xsEndHost(the);	

bail:
	xs_ethernet_destructor(eth);
}

void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	xsEthernet eth = (xsEthernet)(((uint8_t *)cb) - offsetof(xsEthernetRecord, callbackIPv4));
	if (eth->iface != iface)
		return;

	if ((NET_EVENT_IPV4_ADDR_ADD == mgmt_event) ||(NET_EVENT_IPV4_ADDR_DEL == mgmt_event)) {
		atomic_inc(&eth->useCount);
		modMessagePostToMachine(eth->the, (uint8_t *)&mgmt_event, sizeof(mgmt_event), ethernetConnectDeliver, eth);
	}
}

void xs_ethernet_connect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	if (eth->connecting)
		xsUnknownError("already connecting");

	eth->connecting = 1;
	net_dhcpv4_start(eth->iface);
}

void xs_ethernet_disconnect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	eth->connecting = 0;
	eth->connected = 0;
	eth->ipAddress = 0;

	net_dhcpv4_stop(eth->iface);
}

void xs_ethernet_connection_get(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	int connection;
	if (eth->ipAddress)
		connection = 500;
	else if (eth->connected)
		connection = 400;
	else if (eth->connecting)
		connection = 300;
	else
		connection = 200;

	xsmcSetInteger(xsResult, connection);
}

void xs_ethernet_address_get(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	struct in_addr *addr = net_if_ipv4_get_global_addr(eth->iface, NET_ADDR_PREFERRED);
	if (!addr)
		return;

	char addr_str[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET, addr, addr_str, sizeof(addr_str));
	xsmcSetString(xsResult, addr_str);
}

void xs_ethernet_MAC_get(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	struct net_linkaddr *addr = net_if_get_link_addr(eth->iface);
	if ((C_NULL == addr) || (6 != addr->len))
		return;

	static const char hex[] = "0123456789abcdef";
	char mac[18], *s = mac;
	
	for (int i = 0; i < 6; i++) {
		if (i)
			*s++ = ':';
		*s++ = hex[(addr->addr[i] >> 4) & 0x0F];
		*s++ = hex[addr->addr[i] & 0x0F];
	}
	*s = 0;

	xsmcSetString(xsResult, mac);
}
