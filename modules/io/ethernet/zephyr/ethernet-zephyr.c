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
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/hostname.h>

typedef struct {
	uint64_t	mgmt_event;
	uint8_t		addressChanged;
} ZephyrEventMsg;

static uint8_t gStaticIP;

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
	ZephyrEventMsg *msg = (ZephyrEventMsg *)msgIn;
	uint8_t prevConnecting = eth->connecting, prevConnected = eth->connected, prevIpAddress = eth->ipAddress;

	if (eth->closed)
		goto bail;

	if (NET_EVENT_IPV4_ADDR_ADD == msg->mgmt_event) {
		eth->connecting = 0;
		eth->connected = 1;
		eth->ipAddress = 1;
	}
	else if (NET_EVENT_IPV4_ADDR_DEL == msg->mgmt_event) {
		eth->ipAddress = 0;
	}

	if (eth->onChanged) {
		uint8_t connectionChanged = (prevConnecting != eth->connecting) ||
								 (prevConnected != eth->connected) ||
								 (prevIpAddress != eth->ipAddress);
		uint8_t addressChanged = (!prevIpAddress && eth->ipAddress) || msg->addressChanged;
		if (connectionChanged || addressChanged) {
			xsSlot tmp;
			xsBeginHost(the);
			if (connection) {
				xsmcSetStringX(tmp, "connection");
				xsCallFunction1(xsReference(eth->onChanged), eth->obj, tmp);
			}
			if (addressChanged && !eth->closed) {
				xsmcSetStringX(tmp, "address");
				xsCallFunction1(xsReference(eth->onChanged), eth->obj, tmp);
			}
			xsEndHost(the);
		}
	}

bail:
	xs_ethernet_destructor(eth);
}

void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	xsEthernet eth = (xsEthernet)(((uint8_t *)cb) - offsetof(xsEthernetRecord, callbackIPv4));
	if (eth->iface != iface)
		return;

	if ((NET_EVENT_IPV4_ADDR_ADD == mgmt_event) || (NET_EVENT_IPV4_ADDR_DEL == mgmt_event)) {
		ZephyrEventMsg msg = {0};
		msg.mgmt_event = mgmt_event;
		atomic_inc(&eth->useCount);
		modMessagePostToMachine(eth->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, eth);
	}
}

void xs_ethernet_connect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	if (eth->connecting)
		xsUnknownError("already connecting");

	if (gStaticIP) {
		ZephyrEventMsg msg = {0};
		msg.mgmt_event = NET_EVENT_IPV4_ADDR_ADD;
		atomic_inc(&eth->useCount);
		modMessagePostToMachine(eth->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, eth);
	}
	else {
		eth->connecting = 1;
		net_dhcpv4_start(eth->iface);
	}
}

void xs_ethernet_disconnect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	eth->connecting = 0;
	eth->connected = 0;
	eth->ipAddress = 0;

	if (!gStaticIP)
		net_dhcpv4_stop(eth->iface);
}

void xs_ethernet_configure(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	xsmcVars(2);

	if (xsmcHas(xsArg(0), xsID_hostname)) {
		char hostname[64];
		xsmcGet(xsVar(0), xsArg(0), xsID_hostname);
		const char *src = xsmcToStringBuffer(xsVar(0), hostname, sizeof(hostname));
		if (net_hostname_set((char *)src, c_strlen(src)) < 0)
			xsUnknownError("set hostname failed");
	}

	if (xsmcHas(xsArg(0), xsID_static)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_static);
		if (xsmcTest(xsVar(0))) {
			struct in_addr ip, netmask, gw;

			xsmcGet(xsVar(1), xsVar(0), xsID_address);
			if (net_addr_pton(AF_INET, xsmcToString(xsVar(1)), &ip) < 0)
				xsRangeError("invalid address");

			xsmcGet(xsVar(1), xsVar(0), xsID_mask);
			if (net_addr_pton(AF_INET, xsmcToString(xsVar(1)), &netmask) < 0)
				xsRangeError("invalid mask");

			xsmcGet(xsVar(1), xsVar(0), xsID_gateway);
			if (net_addr_pton(AF_INET, xsmcToString(xsVar(1)), &gw) < 0)
				xsRangeError("invalid gateway");

			net_dhcpv4_stop(eth->iface);

			struct in_addr *current = net_if_ipv4_get_global_addr(eth->iface, NET_ADDR_PREFERRED);
			if (current)
				net_if_ipv4_addr_rm(eth->iface, current);

			if (!net_if_ipv4_addr_add(eth->iface, &ip, NET_ADDR_MANUAL, 0))
				xsUnknownError("set IP failed");

			net_if_ipv4_set_netmask_by_addr(eth->iface, &ip, &netmask);
			net_if_ipv4_set_gw(eth->iface, &gw);

			gStaticIP = 1;

			if (eth->connected) {
				ZephyrEventMsg msg = {0};
				msg.mgmt_event = NET_EVENT_IPV4_ADDR_ADD;
				msg.addressChanged = 1;
				atomic_inc(&eth->useCount);
				modMessagePostToMachine(eth->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, eth);
			}
		}
		else {
			struct in_addr *current = net_if_ipv4_get_global_addr(eth->iface, NET_ADDR_PREFERRED);
			if (current)
				net_if_ipv4_addr_rm(eth->iface, current);

			gStaticIP = 0;
			net_dhcpv4_start(eth->iface);
		}
	}
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
