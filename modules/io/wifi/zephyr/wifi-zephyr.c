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
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/hostname.h>

#define NET_EVENT_WIFI_MASK  \
	(NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_CONNECT_RESULT |   \
	NET_EVENT_WIFI_DISCONNECT_RESULT )

#define NET_EVENT_IPv4_MASK  \
	( NET_EVENT_IPV4_DHCP_BOUND )

typedef struct {
	uint64_t	mgmt_event;
	uint8_t	addressChanged;
} ZephyrEventMsg;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
static void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

static void formatMAC(const uint8_t *mac, char *out)
{
	static const char hex[] = "0123456789abcdef";
	for (int i = 0; i < 6; i++) {
		if (i) *out++ = ':';
		*out++ = hex[(mac[i] >> 4) & 0x0F];
		*out++ = hex[mac[i] & 0x0F];
	}
	*out = 0;
}

static const char *securityToString(int security)
{
	switch (security) {
		case WIFI_SECURITY_TYPE_NONE: return "none";
		case WIFI_SECURITY_TYPE_PSK: return "wpa2_psk";
		case WIFI_SECURITY_TYPE_SAE: return "wpa3_psk";
		default: return "unknown";
	}
}

typedef struct {
	xsSlot				obj;
	xsMachine			*the;
	struct net_if		*iface;

	atomic_t				useCount;
	uint8_t				scanning:1;
	uint8_t				connecting:1;
	uint8_t				connected:1;
	uint8_t				dhcpBound:1;
	uint8_t				closed:1;

	xsSlot				*onChanged;
	xsSlot				*onFound;
	xsSlot				*onComplete;

	struct net_mgmt_event_callback callbackWiFi;
	struct net_mgmt_event_callback callbackIPv4;
} xsWiFiRecord, *xsWiFi;

void xs_wifi_destructor(void *data)
{
	xsWiFi wf = (xsWiFi)data;
	if (!wf) return;

	if (atomic_dec(&wf->useCount) > 0)
		return;

	c_free(wf);
}

static void xs_wifi_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	xsWiFi wf = (xsWiFi)it;

	if (wf->onChanged)
		(*markRoot)(the, wf->onChanged);
	if (wf->onFound)
		(*markRoot)(the, wf->onFound);
	if (wf->onComplete)
		(*markRoot)(the, wf->onComplete);
}

static const xsHostHooks xsWiFiHooks = {
	xs_wifi_destructor,
	xs_wifi_mark,
	C_NULL
};

void xs_wifi(xsMachine *the)
{
//@@ port
	struct net_if *iface = net_if_get_wifi_sta();

	if (C_NULL == iface)
		xsUnknownError("no wifi");
	
	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);

	xsWiFi wf = c_calloc(1, sizeof(xsWiFiRecord));
	wf->the = the;
	wf->obj = xsThis;
	atomic_set(&wf->useCount, 1);
	wf->iface = iface;
	wf->onChanged = onChanged;

	xsmcSetHostData(xsThis, wf);
	xsSetHostHooks(xsThis, &xsWiFiHooks);

	xsRemember(wf->obj);

	net_mgmt_init_event_callback(&wf->callbackWiFi, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&wf->callbackWiFi);

	net_mgmt_init_event_callback(&wf->callbackIPv4, ipv4_event_handler, NET_EVENT_IPv4_MASK);
	net_mgmt_add_event_callback(&wf->callbackIPv4);

	struct in_addr *addr = net_if_ipv4_get_global_addr(wf->iface, NET_ADDR_PREFERRED);
	if (addr) {
		wf->connected = 1;
		wf->dhcpBound = 1;
	}
}

void xs_wifi_close(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostData(xsThis);
	if (!wf) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	wf->closed = 1;
	xsForget(wf->obj);
	net_mgmt_del_event_callback(&wf->callbackWiFi);
	net_mgmt_del_event_callback(&wf->callbackIPv4);
	xs_wifi_destructor(wf);
	xsmcSetHostData(xsThis, C_NULL);
	xsSetHostDestructor(xsThis, C_NULL);
}

static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;
	struct wifi_scan_result *entry = (struct wifi_scan_result *)msgIn;

	if (wf->closed)
		goto bail;

	xsBeginHost(the);
		if (C_NULL == entry) {
			wf->scanning = 0;
			if (wf->onComplete)
				xsCallFunction0(xsReference(wf->onComplete), wf->obj);
		}
		else if (wf->onFound) {
			xsmcVars(1);
			xsmcSetNewObject(xsResult);
			xsmcSetString(xsVar(0), entry->ssid);
			xsmcSet(xsResult, xsID_SSID, xsVar(0));
			xsmcSetInteger(xsVar(0), entry->channel);
			xsmcSet(xsResult, xsID_channel, xsVar(0));
			xsmcSetInteger(xsVar(0), entry->rssi);
			xsmcSet(xsResult, xsID_RSSI, xsVar(0));

			char bssidStr[18];
			formatMAC(entry->mac, bssidStr);
			xsmcSetString(xsVar(0), bssidStr);
			xsmcSet(xsResult, xsID_BSSID, xsVar(0));

			xsmcSetStringX(xsVar(0), (char *)securityToString(entry->security));
			xsmcSet(xsResult, xsID_security, xsVar(0));

			xsCallFunction1(xsReference(wf->onFound), wf->obj, xsResult);
		}
	xsEndHost(the);

bail:
	xs_wifi_destructor(wf);
}

static void wifiConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;
	ZephyrEventMsg *msg = (ZephyrEventMsg *)msgIn;
	uint8_t prevConnecting = wf->connecting, prevConnected = wf->connected, prevDhcpBound = wf->dhcpBound;

	if (wf->closed)
		goto bail;

	if (NET_EVENT_WIFI_CONNECT_RESULT == msg->mgmt_event) {
		wf->connecting = 0;
		wf->connected = 1;		//@@ check status
	}
	else if (NET_EVENT_WIFI_DISCONNECT_RESULT == msg->mgmt_event)  {
		wf->connecting = 0;
		wf->connected = 0;
		wf->dhcpBound = 0;
	}
	else if (NET_EVENT_IPV4_DHCP_BOUND == msg->mgmt_event) {
		// this event may be delivered before CONNECT_RESULT
		wf->connecting = 0;
		wf->connected = 1;
		wf->dhcpBound = 1;
	}
	else if (NET_EVENT_IPV4_ADDR_DEL == msg->mgmt_event) {
		// synthetic: IP cleared while link stays up
		wf->dhcpBound = 0;
	}

	if (wf->onChanged) {
		uint8_t connectionChanged = (prevConnecting != wf->connecting) ||
								 (prevConnected != wf->connected) ||
								 (prevDhcpBound != wf->dhcpBound);
		uint8_t addressChanged = (!prevDhcpBound && wf->dhcpBound) || msg->addressChanged;
		if (connectionChanged || addressChanged) {
			xsSlot tmp;
			xsBeginHost(the);
			if (connectionChanged) {
				xsmcSetStringX(tmp, "connection");
				xsCallFunction1(xsReference(wf->onChanged), wf->obj, tmp);
			}
			if (addressChanged && !wf->closed) {
				xsmcSetStringX(tmp, "address");
				xsCallFunction1(xsReference(wf->onChanged), wf->obj, tmp);
			}
			xsEndHost(the);
		}
	}

bail:
	xs_wifi_destructor(wf);
}

void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	xsWiFi wf = (xsWiFi)(((uint8_t *)cb) - offsetof(xsWiFiRecord, callbackWiFi));

	if (NET_EVENT_WIFI_SCAN_RESULT == mgmt_event) {
		const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;

		atomic_inc(&wf->useCount);
		modMessagePostToMachine(wf->the, (void *)entry, sizeof(*entry), wifiScanDeliver, wf);
	}
	else if (NET_EVENT_WIFI_SCAN_DONE == mgmt_event) {
		atomic_inc(&wf->useCount);
		modMessagePostToMachine(wf->the, C_NULL, 0, wifiScanDeliver, wf);
	}
	else if (NET_EVENT_WIFI_CONNECT_RESULT == mgmt_event) {
		const struct wifi_status *status = (const struct wifi_status *) cb->info;
		if (status->status)
			mgmt_event = NET_EVENT_WIFI_DISCONNECT_RESULT;

		ZephyrEventMsg msg = {0};
		msg.mgmt_event = mgmt_event;
		atomic_inc(&wf->useCount);
		modMessagePostToMachine(wf->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, wf);

		if (NET_EVENT_WIFI_CONNECT_RESULT == mgmt_event &&
			net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
			msg.mgmt_event = NET_EVENT_IPV4_DHCP_BOUND;
			atomic_inc(&wf->useCount);
			modMessagePostToMachine(wf->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, wf);
		}
	}
}

void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	xsWiFi wf = (xsWiFi)(((uint8_t *)cb) - offsetof(xsWiFiRecord, callbackIPv4));

	if (NET_EVENT_IPV4_DHCP_BOUND == mgmt_event) {
		ZephyrEventMsg msg = {0};
		msg.mgmt_event = mgmt_event;
		atomic_inc(&wf->useCount);
		modMessagePostToMachine(wf->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, wf);
	}
}

void xs_wifi_scan(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wf->scanning)
		xsUnknownError("already scanning");

	wf->onFound = builtinGetCallback(the, xsID_onFound);
	if (!wf->onFound)
		xsUnknownError("onFound required");
	wf->onComplete = builtinGetCallback(the, xsID_onComplete);

	wf->scanning = 1;
	struct wifi_scan_params params = { 0 };
	params.scan_type = WIFI_SCAN_TYPE_ACTIVE;
	int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wf->iface, &params, sizeof(params));
	if (ret < 0) {
		wf->scanning = 0;
		xsUnknownError("scan failed");
	}
}

void xs_wifi_connect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wf->connecting)
		xsUnknownError("already connecting");

	xsmcVars(1);

	char ssidLocal[WIFI_SSID_MAX_LEN + 1];
	struct wifi_connect_req_params params = { 0 };
	params.band = WIFI_FREQ_BAND_UNKNOWN;
	params.channel = WIFI_CHANNEL_ANY;
	params.security = WIFI_SECURITY_TYPE_NONE;
	params.mfp = WIFI_MFP_OPTIONAL;
	params.eap_ver = 1;
	params.ignore_broadcast_ssid = 0;
	params.bandwidth = WIFI_FREQ_BANDWIDTH_20MHZ;
	params.verify_peer_cert = false;

	if (!xsmcHas(xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");

	xsmcGet(xsVar(0), xsArg(0), xsID_SSID);
	xsmcToStringBuffer(xsVar(0), ssidLocal, sizeof(ssidLocal));
	params.ssid = ssidLocal;
	params.ssid_length = c_strlen(ssidLocal);

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		params.psk = xsmcToString(xsVar(0));
		params.psk_length = c_strlen(params.psk);
		params.security = WIFI_SECURITY_TYPE_PSK;
	}

	wf->connecting = 1;
	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, wf->iface, &params, sizeof(params));
	if (ret < 0) {
		wf->connecting = 0;
		xsUnknownError("connect failed");
	}
}

void xs_wifi_disconnect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wf->iface, C_NULL, 0) < 0)
		xsUnknownError("disconnect failed");

	// if (status == -EALREADY) {		// already disconnected

	wf->connecting = 0;
	wf->connected = 0;
	wf->dhcpBound = 0;
}

void xs_wifi_configure(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

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

			net_dhcpv4_stop(wf->iface);

			struct in_addr *current = net_if_ipv4_get_global_addr(wf->iface, NET_ADDR_PREFERRED);
			if (current)
				net_if_ipv4_addr_rm(wf->iface, current);

			if (!net_if_ipv4_addr_add(wf->iface, &ip, NET_ADDR_MANUAL, 0))
				xsUnknownError("set IP failed");

			net_if_ipv4_set_netmask_by_addr(wf->iface, &ip, &netmask);
			net_if_ipv4_set_gw(wf->iface, &gw);

			if (wf->connected) {
				ZephyrEventMsg msg = {0};
				msg.mgmt_event = NET_EVENT_IPV4_DHCP_BOUND;
				msg.addressChanged = 1;
				atomic_inc(&wf->useCount);
				modMessagePostToMachine(wf->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, wf);
			}
		}
		else {
			struct in_addr *current = net_if_ipv4_get_global_addr(wf->iface, NET_ADDR_PREFERRED);
			if (current)
				net_if_ipv4_addr_rm(wf->iface, current);

			net_dhcpv4_start(wf->iface);

			if (wf->connected) {
				ZephyrEventMsg msg = {0};
				msg.mgmt_event = NET_EVENT_IPV4_ADDR_DEL;
				atomic_inc(&wf->useCount);
				modMessagePostToMachine(wf->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, wf);
			}
		}
	}
}

void xs_wifi_connection_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	int connection;
	if (wf->dhcpBound)
		connection = 500;
	else if (wf->connected)
		connection = 400;
	else if (wf->connecting)
		connection = 300;
	else
		connection = 200;

	xsmcSetInteger(xsResult, connection);
}

void xs_wifi_address_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	struct in_addr *addr = net_if_ipv4_get_global_addr(wf->iface, NET_ADDR_PREFERRED);
	if (!addr)
		return;

	char addr_str[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET, addr, addr_str, sizeof(addr_str));
	xsmcSetString(xsResult, addr_str);
}

void xs_wifi_MAC_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	struct net_linkaddr *addr = net_if_get_link_addr(wf->iface);
	if ((C_NULL == addr) || (6 != addr->len))
		return;

	char mac[18];
	formatMAC(addr->addr, mac);
	xsmcSetString(xsResult, mac);
}

void xs_wifi_SSID_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	if (!wf->connected) return;

	struct wifi_iface_status status = { 0 };
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wf->iface, &status, sizeof(status)))
		return;

	if (status.ssid_len) {
		status.ssid[status.ssid_len] = 0;
		xsmcSetString(xsResult, (char *)status.ssid);
	}
}

void xs_wifi_BSSID_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	if (!wf->connected) return;

	struct wifi_iface_status status = { 0 };
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wf->iface, &status, sizeof(status)))
		return;

	char bssid[18];
	formatMAC(status.bssid, bssid);
	xsmcSetString(xsResult, bssid);
}

void xs_wifi_RSSI_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	if (!wf->connected) return;

	struct wifi_iface_status status = { 0 };
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wf->iface, &status, sizeof(status)))
		return;

	xsmcSetInteger(xsResult, status.rssi);
}

void xs_wifi_channel_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	if (!wf->connected) return;

	struct wifi_iface_status status = { 0 };
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wf->iface, &status, sizeof(status)))
		return;

	xsmcSetInteger(xsResult, status.channel);
}
