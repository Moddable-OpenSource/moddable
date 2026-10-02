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

#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

#include "dhcpserver.h"

#include "modTimer.h"

// Broadcom fullmac WLC ioctl to deauth a specific STA (called WLC_SCB_DEAUTHENTICATE
// in the fullmac driver; the CYW43 driver exposes it as CYW43_IOCTL_SET_DISASSOC 0x69).
// With CYW43_ITF_AP interface and a 6-byte MAC payload, deauths that STA from the AP.

#define AP_POLL_INTERVAL_MS 500
#define AP_MAX_STAS 8		// matches DHCPS_MAX_IP

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

	uint32_t	useCount;
	uint8_t		closed:1;
	uint16_t	connection;

	xsSlot		*onChanged;
	xsSlot		*onConnect;
	xsSlot		*onDisconnect;
	xsSlot		*stationPrototype;

	xsStation	stations;

	dhcp_server_t	dhcpServer;
	modTimer	pollTimer;
} xsAPRecord;

static xsAP gAPInstance;

typedef struct {
	uint8_t		event;			// 1=ready, 2=connected, 3=disconnected
	uint8_t		mac[6];
} APEventMsg;

#define AP_EVT_READY 1
#define AP_EVT_CONNECTED 2
#define AP_EVT_DISCONNECTED 3

static void apPollTimer(modTimer timer, void *refcon, int refconSize);
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

static uint32_t stringToAuthMode(const char *str)
{
	if (0 == c_strcmp(str, "none")) return CYW43_AUTH_OPEN;
	if (0 == c_strcmp(str, "wpa2_psk")) return CYW43_AUTH_WPA2_AES_PSK;
	if (0 == c_strcmp(str, "wpa_wpa2_psk")) return CYW43_AUTH_WPA2_MIXED_PSK;
	if (0 == c_strcmp(str, "wpa3_psk")) return CYW43_AUTH_WPA3_SAE_AES_PSK;
	if (0 == c_strcmp(str, "wpa2_wpa3_psk")) return CYW43_AUTH_WPA3_WPA2_AES_PSK;
	return 0xFFFFFFFF;
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

#ifdef mxDebug
	xsTrace("closed, but disconnect not available on pico\n");
#endif

	uint8_t mac[6];
	c_memcpy(mac, station->mac, sizeof(mac));
	cyw43_ioctl(&cyw43_state, CYW43_IOCTL_SET_DISASSOC, sizeof(mac), mac, CYW43_ITF_AP);
}

void xs_apstation_MAC_get(xsMachine *the)
{
	xsStation station = xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(station->mac, xsmcToString(xsResult));
}

void xs_apstation_address_get(xsMachine *the)
{
	xsStation station = xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	if (!gAPInstance) return;

	// Walk the DHCP server's lease table. The IP for lease[i] is (base IP with last octet DHCPS_BASE_IP + i).
	dhcp_server_t *d = &gAPInstance->dhcpServer;
	for (int i = 0; i < DHCPS_MAX_IP; i++) {
		if ((0 == c_memcmp(d->lease[i].mac, station->mac, sizeof(station->mac)))
			&& d->lease[i].expiry) {
			ip_addr_t leaseIP = d->ip;
			((uint8_t *)&ip_2_ip4(&leaseIP)->addr)[3] = ((uint8_t *)&ip_2_ip4(&d->ip)->addr)[3] + DHCPS_BASE_IP + i;
			xsResult = xsStringBuffer(NULL, 15);
			ipaddr_ntoa_r(&leaseIP, xsmcToString(xsResult), 16);
			return;
		}
	}
}

// -------- WiFiAccessPoint --------

void xs_wifiaccesspoint_destructor(void *data)
{
	xsAP ap = (xsAP)data;
	if (!ap) return;

	ap->useCount -= 1;
	if (ap->useCount > 0)
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

	xsmcVars(1);

	uint8_t ssid[33] = {0};
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");
	xsmcToStringBuffer(xsVar(0), (char *)ssid, sizeof(ssid));
	size_t ssidLen = c_strlen((char *)ssid);
	if (0 == ssidLen)
		xsRangeError("SSID empty");

	uint8_t password[65] = {0};
	uint8_t hasPassword = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_password)) {
		xsmcToStringBuffer(xsVar(0), (char *)password, sizeof(password));
		size_t plen = c_strlen((char *)password);
		if (plen) {
			if (plen < 8)
				xsRangeError("password too short - 8 bytes min");
			hasPassword = 1;
		}
	}

	uint32_t auth = hasPassword ? CYW43_AUTH_WPA3_WPA2_AES_PSK : CYW43_AUTH_OPEN;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_authentication)) {
		char authStr[24];
		xsmcToStringBuffer(xsVar(0), authStr, sizeof(authStr));
		uint32_t mode = stringToAuthMode(authStr);
		if (0xFFFFFFFF == mode)
			xsRangeError("invalid authentication");
		if ((CYW43_AUTH_OPEN != mode) && !hasPassword)
			xsUnknownError("password required for authentication mode");
		auth = mode;
	}

	if (xsmcGet(xsVar(0), xsArg(0), xsID_channel)) {
		int channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsRangeError("invalid channel");
		// CYW43 does not expose an API to set AP channel prior to bring-up; ignore for now
	}

	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);
	xsSlot *onConnect = builtinGetCallback(the, xsID_onConnect);
	xsSlot *onDisconnect = builtinGetCallback(the, xsID_onDisconnect);

	xsAP ap = c_calloc(1, sizeof(xsAPRecord));
	if (!ap)
		xsUnknownError("no memory");

	ap->the = the;
	ap->obj = xsThis;
	ap->useCount = 1;
	ap->connection = 200;
	ap->onChanged = onChanged;
	ap->onConnect = onConnect;
	ap->onDisconnect = onDisconnect;
	ap->stationPrototype = xsmcToReference(xsArg(1));

	xsmcSetHostData(xsThis, ap);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAPHooks);
	xsRemember(ap->obj);

	int err = pico_use_cyw43();
	if (err)
		xsUnknownError("CYW43 init failed %d", err);

	cyw43_arch_enable_ap_mode((const char *)ssid, hasPassword ? (const char *)password : NULL, auth);

	// Configure AP netif address and start DHCP server on 192.168.4.1/24
	ip_addr_t ip, nm;
	IP4_ADDR(ip_2_ip4(&ip), 192, 168, 4, 1);
	IP4_ADDR(ip_2_ip4(&nm), 255, 255, 255, 0);

	struct netif *apnetif = &cyw43_state.netif[CYW43_ITF_AP];
	netif_set_addr(apnetif, ip_2_ip4(&ip), ip_2_ip4(&nm), ip_2_ip4(&ip));

	/* Bind to the AP netif so a later station connection does not answer DHCP. */
	dhcp_server_init(&ap->dhcpServer, apnetif, &ip, &nm);

	gAPInstance = ap;
	ap->connection = 400;

	// Fire ready and start polling for client changes
	if (ap->onChanged) {
		APEventMsg m = {0};
		m.event = AP_EVT_READY;
		ap->useCount += 1;
		modMessagePostToMachine(the, (uint8_t *)&m, sizeof(m), apEventDeliver, ap);
	}

	if (ap->onConnect || ap->onDisconnect)
		ap->pollTimer = modTimerAdd(AP_POLL_INTERVAL_MS, AP_POLL_INTERVAL_MS, apPollTimer, NULL, 0);
}

static void apPollTimer(modTimer timer, void *refcon, int refconSize)
{
	xsAP ap = gAPInstance;
	if (!ap) return;

	int numStas = 0;
	uint8_t macs[AP_MAX_STAS * 6] = {0};
	cyw43_wifi_ap_get_max_stas(&cyw43_state, &numStas);
	if (numStas > AP_MAX_STAS) numStas = AP_MAX_STAS;
	cyw43_wifi_ap_get_stas(&cyw43_state, &numStas, macs);

	// Detect new connections
	for (int i = 0; i < numStas; i++) {
		uint8_t *m = macs + (i * 6);
		if (!findStationByMAC(ap, m)) {
			APEventMsg msg = {0};
			msg.event = AP_EVT_CONNECTED;
			c_memcpy(msg.mac, m, 6);
			ap->useCount += 1;
			modMessagePostToMachine(ap->the, (uint8_t *)&msg, sizeof(msg), apEventDeliver, ap);
		}
	}

	// Detect disconnections
	xsStation walker = ap->stations;
	while (walker) {
		xsStation next = walker->next;
		uint8_t stillPresent = 0;
		for (int i = 0; i < numStas; i++) {
			if (0 == c_memcmp(macs + (i * 6), walker->mac, 6)) {
				stillPresent = 1;
				break;
			}
		}
		if (!stillPresent) {
			APEventMsg msg = {0};
			msg.event = AP_EVT_DISCONNECTED;
			c_memcpy(msg.mac, walker->mac, 6);
			ap->useCount += 1;
			modMessagePostToMachine(ap->the, (uint8_t *)&msg, sizeof(msg), apEventDeliver, ap);
		}
		walker = next;
	}
}

static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsAP ap = refcon;
	APEventMsg *msg = (APEventMsg *)msgIn;

	if (ap->closed)
		goto bail;

	if (AP_EVT_READY == msg->event) {
		if (!ap->onChanged) goto bail;
		xsBeginHost(the);
			xsmcSetStringX(xsResult, (char *)"connection");
			xsCallFunction1(xsReference(ap->onChanged), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (AP_EVT_CONNECTED == msg->event) {
		// Guard against double-fire from overlapping polls
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

			if (ap->onConnect)
				xsCallFunction1(xsReference(ap->onConnect), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (AP_EVT_DISCONNECTED == msg->event) {
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
		if (ap->pollTimer) {
			modTimerRemove(ap->pollTimer);
			ap->pollTimer = NULL;
		}

		// this doesn't seem to do anything
		for (xsStation walker = ap->stations; walker; walker = walker->next) {
			uint8_t mac[6];
			c_memcpy(mac, walker->mac, sizeof(mac));
			cyw43_ioctl(&cyw43_state, CYW43_IOCTL_SET_DISASSOC, sizeof(mac), mac, CYW43_ITF_AP);
		}

		dhcp_server_deinit(&ap->dhcpServer);
		cyw43_arch_disable_ap_mode();

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
}

void xs_wifiaccesspoint_connection_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	xsmcSetInteger(xsResult, ap->connection);
}

void xs_wifiaccesspoint_address_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	struct netif *apnetif = &cyw43_state.netif[CYW43_ITF_AP];
	const ip4_addr_t *ip = netif_ip4_addr(apnetif);
	if (!ip->addr) return;

	xsResult = xsStringBuffer(NULL, 15);
	ipaddr_ntoa_r(ip, xsmcToString(xsResult), 16);
}

void xs_wifiaccesspoint_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	uint8_t mac[6];

	if (0 != cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifiaccesspoint_SSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	size_t len = cyw43_state.ap_ssid_len;
	if (!len || (len > sizeof(cyw43_state.ap_ssid))) return;

	char ssid[sizeof(cyw43_state.ap_ssid) + 1];
	c_memcpy(ssid, cyw43_state.ap_ssid, len);
	ssid[len] = 0;
	xsmcSetString(xsResult, ssid);
}

void xs_wifiaccesspoint_channel_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	uint32_t channel;

	if (0 == cyw43_ioctl(&cyw43_state, CYW43_IOCTL_GET_CHANNEL, sizeof(channel), (uint8_t *)&channel, CYW43_ITF_AP))
		xsmcSetInteger(xsResult, channel);
}
