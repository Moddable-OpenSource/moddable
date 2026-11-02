/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "mc.xs.h"
#include "builtinCommon.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

#define AP_EVENTS_MASK  \
	(NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
	 NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

typedef struct xsStationRecord *xsStation;
typedef struct xsStationRecord {
	xsStation		next;
	xsSlot			*obj;
	uint8_t			mac[6];
} xsStationRecord;

typedef struct xsAPRecord *xsAP;
typedef struct xsAPRecord {
	xsSlot		obj;
	xsMachine	*the;
	struct net_if	*iface;

	atomic_t	useCount;
	uint8_t		closed:1;
	uint16_t	connection;

	xsSlot		*onChanged;
	xsSlot		*onConnect;
	xsSlot		*onDisconnect;
	xsSlot		*stationPrototype;

	xsStation	stations;

	struct net_mgmt_event_callback	eventCB;
} xsAPRecord;

static xsAP gAPInstance;

typedef struct {
	uint64_t	event;
	uint8_t		mac[6];
} APEventMsg;

static void doAPEvent(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface);
static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);

static void formatMAC(const uint8_t *mac, char *str)
{
	static const char hex[] = "0123456789abcdef";
	for (int i = 0; i < 6; i++) {
		if (i) *str++ = ':';
		*str++ = hex[(mac[i] >> 4) & 0x0F];
		*str++ = hex[mac[i] & 0x0F];
	}
	*str = 0;
}

static enum wifi_security_type stringToSecurity(const char *str, uint8_t *ok)
{
	*ok = 1;
	if (0 == c_strcmp(str, "none")) return WIFI_SECURITY_TYPE_NONE;
	if (0 == c_strcmp(str, "wpa2_psk")) return WIFI_SECURITY_TYPE_PSK;
	if (0 == c_strcmp(str, "wpa3_psk")) return WIFI_SECURITY_TYPE_SAE;
	if (0 == c_strcmp(str, "wpa2_wpa3_psk")) return WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL;
	*ok = 0;
	return WIFI_SECURITY_TYPE_NONE;
}

static xsStation findStationByMAC(xsAP ap, const uint8_t *mac)
{
	for (xsStation walker = ap->stations; walker; walker = walker->next) {
		if (0 == c_memcmp(walker->mac, mac, sizeof(walker->mac)))
			return walker;
	}
	return C_NULL;
}

// -------- Station --------

void xs_apstation_destructor(void *data)
{
	xsStation station = (xsStation)data;
	if (!station) return;
	c_free(station);
}

static void xs_apstation_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
}

static const xsHostHooks xsStationHooks = {
	xs_apstation_destructor,
	xs_apstation_mark,
	C_NULL
};

void xs_apstation_close(xsMachine *the)
{
	xsStation station = xsmcGetHostData(xsThis);
	if (!station) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	if (gAPInstance && gAPInstance->iface) {
		uint8_t mac[6];
		c_memcpy(mac, station->mac, sizeof(mac));
		net_mgmt(NET_REQUEST_WIFI_AP_STA_DISCONNECT, gAPInstance->iface, mac, sizeof(mac));
	}
	// removal from AP list and onDisconnect happen from the AP_STA_DISCONNECTED event
}

void xs_apstation_MAC_get(xsMachine *the)
{
	xsStation station = xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(station->mac, xsmcToString(xsResult));
}

struct leaseSearch {
	const uint8_t *mac;
	struct in_addr ip;
	uint8_t found;
};

static void leaseSearchCB(struct net_if *iface, struct dhcpv4_addr_slot *lease, void *user_data)
{
	struct leaseSearch *s = (struct leaseSearch *)user_data;
	if (s->found) return;
	if (lease->state != DHCPV4_SERVER_ADDR_ALLOCATED) return;

	// DHCP client_id for Ethernet clients is 1 type byte (0x01) + 6-byte MAC.
	if ((lease->client_id.len >= 7) && (0x01 == lease->client_id.buf[0])
		&& (0 == c_memcmp(&lease->client_id.buf[1], s->mac, 6))) {
		s->ip = lease->addr;
		s->found = 1;
	}
}

void xs_apstation_address_get(xsMachine *the)
{
	xsStation station = xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	if (!gAPInstance || !gAPInstance->iface) return;

	struct leaseSearch search = { .mac = station->mac };
	net_dhcpv4_server_foreach_lease(gAPInstance->iface, leaseSearchCB, &search);
	if (!search.found) return;

	char addr[NET_IPV4_ADDR_LEN];
	if (!net_addr_ntop(AF_INET, &search.ip, addr, sizeof(addr))) return;
	xsmcSetString(xsResult, addr);
}

// -------- WiFiAccessPoint --------

void xs_wifiaccesspoint_destructor(void *data)
{
	xsAP ap = (xsAP)data;
	if (!ap) return;

	if (atomic_dec(&ap->useCount) > 1)
		return;

	c_free(ap);
}

static void xs_wifiaccesspoint_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
	xsAP ap = (xsAP)it;
	if (ap->onChanged)
		(*markRoot)(the, ap->onChanged);
	if (ap->onConnect)
		(*markRoot)(the, ap->onConnect);
	if (ap->onDisconnect)
		(*markRoot)(the, ap->onDisconnect);
	if (ap->stationPrototype)
		(*markRoot)(the, ap->stationPrototype);
	for (xsStation walker = ap->stations; walker; walker = walker->next)
		(*markRoot)(the, walker->obj);
}

static const xsHostHooks xsAPHooks = {
	xs_wifiaccesspoint_destructor,
	xs_wifiaccesspoint_mark,
	C_NULL
};

void xs_wifiaccesspoint(xsMachine *the)
{
	if (gAPInstance)
		xsUnknownError("access point already open");

	struct net_if *iface = net_if_get_wifi_sap();
	if (C_NULL == iface)
		xsUnknownError("no soft-AP interface");

	xsmcVars(1);

	uint8_t ssidBuf[WIFI_SSID_MAX_LEN + 1] = {0};
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");
	xsmcToStringBuffer(xsVar(0), (char *)ssidBuf, sizeof(ssidBuf));
	size_t ssidLen = c_strlen((char *)ssidBuf);
	if (0 == ssidLen)
		xsRangeError("SSID empty");

	uint8_t pskBuf[65] = {0};
	uint8_t hasPassword = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_password)) {
		xsmcToStringBuffer(xsVar(0), (char *)pskBuf, sizeof(pskBuf));
		size_t plen = c_strlen((char *)pskBuf);
		if (plen) {
			if (plen < 8)
				xsRangeError("password too short - 8 bytes min");
			hasPassword = 1;
		}
	}

	enum wifi_security_type security = hasPassword ? WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL : WIFI_SECURITY_TYPE_NONE;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_authentication)) {
		char authStr[24];
		xsmcToStringBuffer(xsVar(0), authStr, sizeof(authStr));
		uint8_t ok;
		enum wifi_security_type mode = stringToSecurity(authStr, &ok);
		if (!ok)
			xsRangeError("invalid authentication");
		if ((WIFI_SECURITY_TYPE_NONE != mode) && !hasPassword)
			xsUnknownError("password required for authentication mode");
		security = mode;
	}

	uint8_t channel = WIFI_CHANNEL_ANY;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_channel)) {
		int c = xsmcToInteger(xsVar(0));
		if ((c < 1) || (c > 13))
			xsRangeError("invalid channel");
		channel = c;
	}

	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);
	xsSlot *onConnect = builtinGetCallback(the, xsID_onConnect);
	xsSlot *onDisconnect = builtinGetCallback(the, xsID_onDisconnect);

	xsAP ap = c_calloc(1, sizeof(xsAPRecord));
	if (!ap)
		xsUnknownError("no memory");

	ap->the = the;
	ap->obj = xsThis;
	ap->iface = iface;
	atomic_set(&ap->useCount, 1);
	ap->connection = 200;
	ap->onChanged = onChanged;
	ap->onConnect = onConnect;
	ap->onDisconnect = onDisconnect;
	ap->stationPrototype = xsmcToReference(xsArg(1));

	xsmcSetHostData(xsThis, ap);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAPHooks);
	xsRemember(ap->obj);

	net_mgmt_init_event_callback(&ap->eventCB, doAPEvent, AP_EVENTS_MASK);
	net_mgmt_add_event_callback(&ap->eventCB);

	gAPInstance = ap;

	struct wifi_connect_req_params params = {0};
	params.ssid = ssidBuf;
	params.ssid_length = ssidLen;
	params.band = WIFI_FREQ_BAND_UNKNOWN;
	params.channel = channel;
	params.security = security;
	params.mfp = WIFI_MFP_OPTIONAL;
	params.timeout = SYS_FOREVER_MS;
	if (hasPassword) {
		params.psk = pskBuf;
		params.psk_length = c_strlen((char *)pskBuf);
	}

	int err = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));
	if (err) {
		gAPInstance = C_NULL;
		net_mgmt_del_event_callback(&ap->eventCB);
		xsUnknownError("AP enable failed");
	}

	// Start DHCPv4 server on 192.168.4.10 (base) — pool size from CONFIG_NET_DHCPV4_SERVER_ADDR_COUNT.
	struct in_addr baseAddr;
	if (net_addr_pton(AF_INET, "192.168.4.10", &baseAddr) == 0)
		net_dhcpv4_server_start(iface, &baseAddr);
}

static void doAPEvent(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	xsAP ap = gAPInstance;
	if (!ap) return;
	if (iface != ap->iface) return;

	APEventMsg msg = {0};
	msg.event = event;

	if (NET_EVENT_WIFI_AP_ENABLE_RESULT == event) {
		if (400 == ap->connection) return;
		ap->connection = 400;
		if (!ap->onChanged) return;
	}
	else if (NET_EVENT_WIFI_AP_DISABLE_RESULT == event) {
		if (200 == ap->connection) return;
		ap->connection = 200;
		if (!ap->onChanged) return;
	}
	else if (NET_EVENT_WIFI_AP_STA_CONNECTED == event) {
		if (!ap->onConnect) return;
		const struct wifi_ap_sta_info *info = (const struct wifi_ap_sta_info *)cb->info;
		c_memcpy(msg.mac, info->mac, sizeof(msg.mac));
	}
	else if (NET_EVENT_WIFI_AP_STA_DISCONNECTED == event) {
		const struct wifi_ap_sta_info *info = (const struct wifi_ap_sta_info *)cb->info;
		c_memcpy(msg.mac, info->mac, sizeof(msg.mac));
	}
	else
		return;

	atomic_inc(&ap->useCount);
	modMessagePostToMachine(ap->the, (uint8_t *)&msg, sizeof(msg), apEventDeliver, ap);
}

static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsAP ap = refcon;
	APEventMsg *msg = (APEventMsg *)msgIn;

	if (ap->closed)
		goto bail;

	if ((NET_EVENT_WIFI_AP_ENABLE_RESULT == msg->event) || (NET_EVENT_WIFI_AP_DISABLE_RESULT == msg->event)) {
		xsBeginHost(the);
			xsmcSetStringX(xsResult, (char *)"connection");
			xsCallFunction1(xsReference(ap->onChanged), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (NET_EVENT_WIFI_AP_STA_CONNECTED == msg->event) {
		if (findStationByMAC(ap, msg->mac))
			goto bail;

		xsBeginHost(the);
			xsStation station = c_calloc(1, sizeof(xsStationRecord));
			if (!station)
				xsUnknownError("no memory");
			c_memcpy(station->mac, msg->mac, sizeof(station->mac));

			xsResult = xsNewHostInstance(xsReference(ap->stationPrototype));
			station->obj = xsmcToReference(xsResult);
			xsmcSetHostData(xsResult, station);
			xsSetHostHooks(xsResult, (xsHostHooks *)&xsStationHooks);

			station->next = ap->stations;
			ap->stations = station;

			xsCallFunction1(xsReference(ap->onConnect), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (NET_EVENT_WIFI_AP_STA_DISCONNECTED == msg->event) {
		xsStation station = findStationByMAC(ap, msg->mac);
		if (!station)
			goto bail;

		if (ap->stations == station)
			ap->stations = station->next;
		else {
			for (xsStation walker = ap->stations; walker; walker = walker->next) {
				if (walker->next == station) {
					walker->next = station->next;
					break;
				}
			}
		}
		station->next = C_NULL;

		xsBeginHost(the);
			if (ap->onDisconnect)
				xsCallFunction1(xsReference(ap->onDisconnect), ap->obj, xsReference(station->obj));

			xsResult = xsReference(station->obj);
			xsmcSetHostData(xsResult, C_NULL);
			xsSetHostDestructor(xsResult, C_NULL);
			c_free(station);
		xsEndHost(the);
	}

bail:
	xs_wifiaccesspoint_destructor(ap);
}

void xs_wifiaccesspoint_close(xsMachine *the)
{
	xsAP ap = xsmcGetHostData(xsThis);
	if (!ap) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	ap->closed = 1;
	xsForget(ap->obj);

	if (gAPInstance == ap) {
		// deauth all associated stations so clients notice we're going away
		for (xsStation walker = ap->stations; walker; walker = walker->next) {
			uint8_t mac[6];
			c_memcpy(mac, walker->mac, sizeof(mac));
			net_mgmt(NET_REQUEST_WIFI_AP_STA_DISCONNECT, ap->iface, mac, sizeof(mac));
		}

		net_dhcpv4_server_stop(ap->iface);
		net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap->iface, C_NULL, 0);
		net_mgmt_del_event_callback(&ap->eventCB);
		gAPInstance = C_NULL;
	}

	while (ap->stations) {
		xsStation station = ap->stations;
		ap->stations = station->next;
		xsSlot ref = xsReference(station->obj);
		xsmcSetHostData(ref, C_NULL);
		xsSetHostDestructor(ref, C_NULL);
		c_free(station);
	}

	xs_wifiaccesspoint_destructor(ap);
	xsmcSetHostData(xsThis, C_NULL);
	xsSetHostDestructor(xsThis, C_NULL);
}

void xs_wifiaccesspoint_configure(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	xsUnknownError("configure requires restart");
}

void xs_wifiaccesspoint_connection_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	xsmcSetInteger(xsResult, ap->connection);
}

void xs_wifiaccesspoint_address_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	struct in_addr *addr = net_if_ipv4_get_global_addr(ap->iface, NET_ADDR_PREFERRED);
	if (!addr) return;

	char addr_str[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET, addr, addr_str, sizeof(addr_str));
	xsmcSetString(xsResult, addr_str);
}

void xs_wifiaccesspoint_MAC_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	struct net_linkaddr *link = net_if_get_link_addr(ap->iface);
	if ((C_NULL == link) || (6 != link->len)) return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(link->addr, xsmcToString(xsResult));
}

void xs_wifiaccesspoint_SSID_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	struct wifi_iface_status status = {0};
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, ap->iface, &status, sizeof(status)))
		return;

	if (status.ssid_len) {
		status.ssid[status.ssid_len] = 0;
		xsmcSetString(xsResult, (char *)status.ssid);
	}
}

void xs_wifiaccesspoint_channel_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	struct wifi_iface_status status = {0};
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, ap->iface, &status, sizeof(status)))
		return;

	xsmcSetInteger(xsResult, status.channel);
}
