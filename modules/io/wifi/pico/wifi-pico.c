/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
#include "lwip/dhcp.h"

#include "modTimer.h"

enum {
	PICO_EVENT_CONNECTED = 1,
	PICO_EVENT_DISCONNECTED = 2,
	PICO_EVENT_GOT_IP = 3
};

typedef struct {
	int32_t		event;
	uint8_t		addressChanged;
} WiFiEventMsg;

typedef struct WiFiScanResultNode *WiFiScanResultList;
typedef struct WiFiScanResultNode {
	WiFiScanResultList	next;
	uint8_t				bssid[6];
	uint8_t				channel;
	int16_t				rssi;
	uint32_t			authmode;
	char				ssid[33];
} WiFiScanResultNode;

typedef struct xsWiFiRecord *xsWiFi;
typedef struct xsWiFiRecord {
	xsWiFi		next;
	xsSlot		obj;
	xsMachine	*the;

	uint32_t		useCount;
	uint8_t		scanning:1;
	uint8_t		connecting:1;
	uint8_t		connected:1;
	uint8_t		closed:1;
	uint8_t		gotIP:1;

	xsSlot		*onChanged;
	xsSlot		*onFound;
	xsSlot		*onComplete;
} xsWiFiRecord;

static xsWiFi gWiFiList;
static xsWiFi gScan;
static uint8_t gInited;
static modTimer gActivityTimer;
static WiFiScanResultList gScanResults;
static char gHostname[64];

static void wifiLinkCallback(struct netif *netif);
static void wifiStatusCallback(struct netif *netif);
static void activityTimerCallback(modTimer timer, void *refcon, int refconSize);
static void wifiConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void initWiFi(xsMachine *the);

static void formatMAC(const uint8_t *mac, char *str)
{
	static const char hex[] = "0123456789abcdef";
	char *s = str;
	for (int i = 0; i < 6; i++) {
		if (i) *s++ = ':';
		*s++ = hex[(mac[i] >> 4) & 0x0F];
		*s++ = hex[mac[i] & 0x0F];
	}
	*s = 0;
}

static const char *authmodeToString(uint32_t authmode)
{
	switch (authmode) {
		case CYW43_AUTH_OPEN:			return "none";
		case CYW43_AUTH_WPA_TKIP_PSK:	return "wpa_psk";
		case CYW43_AUTH_WPA2_AES_PSK:	return "wpa2_psk";
		case CYW43_AUTH_WPA2_MIXED_PSK:	return "wpa_wpa2_psk";
		default:						return "unknown";
	}
}

static void freeScanResults(void)
{
	WiFiScanResultNode *node;
	while (NULL != (node = gScanResults)) {
		gScanResults = node->next;
		c_free(node);
	}
}

static void broadcastWiFiEvent(WiFiEventMsg *msg)
{
	for (xsWiFi walker = gWiFiList; walker; walker = walker->next) {
		walker->useCount += 1;
		modMessagePostToMachine(walker->the, (uint8_t *)msg, sizeof(*msg), wifiConnectDeliver, walker);
	}
}

static void startActivityTimer(void)
{
	if (!gActivityTimer)
		gActivityTimer = modTimerAdd(250, 250, activityTimerCallback, NULL, 0);
}

static void stopActivityTimer(void)
{
	if (gActivityTimer) {
		modTimerRemove(gActivityTimer);
		gActivityTimer = NULL;
	}
}

void xs_wifi419_destructor(void *data)
{
	xsWiFi wf = (xsWiFi)data;
	if (!wf) return;

	wf->useCount -= 1;
	if (wf->useCount > 0)
		return;

	c_free(wf);
}

static void xs_wifi419_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
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
	xs_wifi419_destructor,
	xs_wifi419_mark,
	C_NULL
};

void xs_wifi419(xsMachine *the)
{
	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);

	xsWiFi wf = c_calloc(1, sizeof(xsWiFiRecord));
	if (!wf)
		xsUnknownError("no memory");

	wf->the = the;
	wf->obj = xsThis;
	wf->useCount = 1;
	wf->onChanged = onChanged;

	xsmcSetHostData(xsThis, wf);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsWiFiHooks);
	xsRemember(wf->obj);

	initWiFi(the);

	int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
	if (CYW43_LINK_UP == status) {
		wf->connected = 1;
		wf->gotIP = 1;
	}
	else if (CYW43_LINK_JOIN == status || CYW43_LINK_NOIP == status) {
		wf->connected = 1;
	}

	wf->next = gWiFiList;
	gWiFiList = wf;
}

void xs_wifi419_close(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostData(xsThis);
	if (!wf) return;
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	wf->closed = 1;
	xsForget(wf->obj);

	if (gScan == wf) {
		gScan = C_NULL;
		wf->scanning = 0;
		freeScanResults();
	}

	if (gWiFiList == wf)
		gWiFiList = wf->next;
	else {
		for (xsWiFi walker = gWiFiList; walker; walker = walker->next) {
			if (walker->next == wf) {
				walker->next = wf->next;
				break;
			}
		}
	}

	xs_wifi419_destructor(wf);
	xsmcSetHostData(xsThis, C_NULL);
	xsSetHostDestructor(xsThis, C_NULL);
}

static void initWiFi(xsMachine *the)
{
	if (gInited) return;

	int err = pico_use_cyw43();
	if (err)
		xsUnknownError("CYW43 init failed %d", err);

	cyw43_arch_enable_sta_mode();

	netif_set_link_callback(netif_list, wifiLinkCallback);
	netif_set_status_callback(netif_list, wifiStatusCallback);

	gInited = 1;
}

static void wifiLinkCallback(struct netif *netif)
{
	WiFiEventMsg msg = {0};
	msg.event = netif_is_link_up(netif) ? PICO_EVENT_CONNECTED : PICO_EVENT_DISCONNECTED;
	broadcastWiFiEvent(&msg);
}

static void wifiStatusCallback(struct netif *netif)
{
	if (netif_is_link_up(netif) && netif_ip4_addr(netif)->addr) {
		WiFiEventMsg msg = {0};
		msg.event = PICO_EVENT_GOT_IP;
		msg.addressChanged = 1;
		broadcastWiFiEvent(&msg);
	}
}

static int scanResultCallback(void *env, const cyw43_ev_scan_result_t *result)
{
	if (!result || !gScan) return 0;

	for (WiFiScanResultNode *existing = gScanResults; existing; existing = existing->next) {
		if (0 == c_memcmp(existing->bssid, result->bssid, 6)) {
			if (result->rssi > existing->rssi)
				existing->rssi = result->rssi;
			return 0;
		}
	}

	WiFiScanResultNode *node = c_malloc(sizeof(WiFiScanResultNode));
	if (!node) return 0;

	uint8_t len = result->ssid_len;
	if (len > 32) len = 32;
	c_memcpy(node->ssid, result->ssid, len);
	node->ssid[len] = 0;
	c_memcpy(node->bssid, result->bssid, 6);
	node->rssi = result->rssi;
	node->channel = result->channel;
	node->authmode = result->auth_mode;
	node->next = gScanResults;
	gScanResults = node;

	return 0;
}

static void activityTimerCallback(modTimer timer, void *refcon, int refconSize)
{
	if (gScan && !cyw43_wifi_scan_active(&cyw43_state)) {
		xsWiFi wf = gScan;
		gScan = C_NULL;
		wf->useCount += 1;
		modMessagePostToMachine(wf->the, C_NULL, 0, wifiScanDeliver, wf);
	}

	uint8_t anyConnecting = 0;
	for (xsWiFi walker = gWiFiList; walker && !anyConnecting; walker = walker->next)
		anyConnecting = walker->connecting;

	if (anyConnecting && cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) < 0) {
		WiFiEventMsg msg = {0};
		msg.event = PICO_EVENT_DISCONNECTED;
		broadcastWiFiEvent(&msg);
	}

	if (!gScan && !anyConnecting)
		stopActivityTimer();
}

static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;

	if (wf->closed || !wf->scanning)
		goto bail;

	wf->scanning = 0;

	xsBeginHost(the);

	xsmcVars(1);
	WiFiScanResultNode *node;
	while (NULL != (node = gScanResults)) {
		gScanResults = node->next;

		xsTry {
			xsmcSetNewObject(xsResult);

			xsmcSetString(xsVar(0), node->ssid);
			xsmcSet(xsResult, xsID_SSID, xsVar(0));

			xsVar(0) = xsStringBuffer(NULL, 17);
			formatMAC(node->bssid, xsmcToString(xsVar(0)));
			xsmcSet(xsResult, xsID_BSSID, xsVar(0));

			xsmcSetInteger(xsVar(0), node->rssi);
			xsmcSet(xsResult, xsID_RSSI, xsVar(0));

			xsmcSetInteger(xsVar(0), node->channel);
			xsmcSet(xsResult, xsID_channel, xsVar(0));

			xsmcSetStringX(xsVar(0), (char *)authmodeToString(node->authmode));
			xsmcSet(xsResult, xsID_security, xsVar(0));

			xsCallFunction1(xsReference(wf->onFound), wf->obj, xsResult);
		}
		xsCatch {
		}

		c_free(node);
	}

	if (wf->onComplete)
		xsCallFunction0(xsReference(wf->onComplete), wf->obj);

	xsEndHost(the);

bail:
	freeScanResults();
	xs_wifi419_destructor(wf);
}

static void wifiConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;
	WiFiEventMsg *msg = (WiFiEventMsg *)msgIn;
	uint8_t prevConnecting = wf->connecting, prevConnected = wf->connected, prevIP = wf->gotIP;

	if (wf->closed)
		goto bail;

	if (PICO_EVENT_CONNECTED == msg->event) {
		wf->connecting = 0;
		wf->connected = 1;
		wf->gotIP = 0;
	}
	else if (PICO_EVENT_DISCONNECTED == msg->event) {
		wf->connecting = 0;
		wf->connected = 0;
		wf->gotIP = 0;
	}
	else if (PICO_EVENT_GOT_IP == msg->event) {
		wf->connected = 1;
		wf->gotIP = 1;
	}

	if (wf->onChanged) {
		uint8_t connection = (prevConnecting != wf->connecting) ||
								 (prevConnected != wf->connected) ||
								 (prevIP != wf->gotIP);
		if (connection || msg->addressChanged) {
			xsSlot tmp;
			xsBeginHost(the);
			if (connection) {
				xsmcSetStringX(tmp, "connection");
				xsCallFunction1(xsReference(wf->onChanged), wf->obj, tmp);
			}
			if (msg->addressChanged && !wf->closed) {
				xsmcSetStringX(tmp, "address");
				xsCallFunction1(xsReference(wf->onChanged), wf->obj, tmp);
			}
			xsEndHost(the);
		}
	}

bail:
	xs_wifi419_destructor(wf);
}

void xs_wifi419_scan(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (gScan)
		xsUnknownError("already scanning");

	wf->onFound = builtinGetCallback(the, xsID_onFound);
	if (!wf->onFound)
		xsUnknownError("onFound required");
	wf->onComplete = builtinGetCallback(the, xsID_onComplete);

	freeScanResults();

	cyw43_wifi_scan_options_t scan_options = {0};
	wf->scanning = 1;
	gScan = wf;
	int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scanResultCallback);
	if (err) {
		wf->scanning = 0;
		gScan = C_NULL;
		xsUnknownError("scan failed %d", err);
	}

	startActivityTimer();
}

void xs_wifi419_connect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wf->connecting)
		xsUnknownError("already connecting");

	uint8_t ssid[33] = {0};
	uint8_t password[65] = {0};
	uint32_t auth = CYW43_AUTH_OPEN;

	xsmcVars(1);
	if (!xsmcHas(xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");

	xsmcGet(xsVar(0), xsArg(0), xsID_SSID);
	xsmcToStringBuffer(xsVar(0), (char *)ssid, sizeof(ssid));

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		xsmcToStringBuffer(xsVar(0), (char *)password, sizeof(password));
		auth = CYW43_AUTH_WPA2_AES_PSK;
	}

	int err = cyw43_arch_wifi_connect_async((const char *)ssid, (const char *)password, auth);
	if (err)
		xsUnknownError("connect failed %d", err);

	wf->connecting = 1;

	startActivityTimer();
}

void xs_wifi419_disconnect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);

	wf->connecting = 0;
	wf->connected = 0;
	wf->gotIP = 0;
}

void xs_wifi419_configure(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (!gInited)
		xsUnknownError("not initialized");

	xsmcVars(2);

	if (xsmcHas(xsArg(0), xsID_hostname)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hostname);
		xsmcToStringBuffer(xsVar(0), gHostname, sizeof(gHostname));
		netif_set_hostname(netif_list, gHostname);
	}

	if (xsmcHas(xsArg(0), xsID_static)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_static);
		if (xsmcTest(xsVar(0))) {
			ip4_addr_t ip, netmask, gw;

			xsmcGet(xsVar(1), xsVar(0), xsID_address);
			if (!ip4addr_aton(xsmcToString(xsVar(1)), &ip))
				xsRangeError("invalid address");

			xsmcGet(xsVar(1), xsVar(0), xsID_mask);
			if (!ip4addr_aton(xsmcToString(xsVar(1)), &netmask))
				xsRangeError("invalid mask");

			xsmcGet(xsVar(1), xsVar(0), xsID_gateway);
			if (!ip4addr_aton(xsmcToString(xsVar(1)), &gw))
				xsRangeError("invalid gateway");

			dhcp_stop(netif_list);
			netif_set_addr(netif_list, &ip, &netmask, &gw);

			if (netif_is_link_up(netif_list)) {
				WiFiEventMsg msg = {0};
				msg.event = PICO_EVENT_GOT_IP;
				msg.addressChanged = 1;
				broadcastWiFiEvent(&msg);
			}
		}
		else {
			ip4_addr_t zero = {0};
			netif_set_addr(netif_list, &zero, &zero, &zero);
			dhcp_start(netif_list);

			if (netif_is_link_up(netif_list)) {
				WiFiEventMsg msg = {0};
				msg.event = PICO_EVENT_CONNECTED;
				broadcastWiFiEvent(&msg);
			}
		}
	}
}

void xs_wifi419_connection_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	int connection;

	if (wf->connected)
		connection = wf->gotIP ? 500 : 400;
	else if (wf->connecting)
		connection = 300;
	else
		connection = 200;

	xsmcSetInteger(xsResult, connection);
}

void xs_wifi419_address_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (!netif_is_up(netif_list))
		return;

	const ip4_addr_t *ip = netif_ip4_addr(netif_list);
	if (!ip->addr)
		return;

	xsResult = xsStringBuffer(NULL, 15);
	ipaddr_ntoa_r(ip, xsmcToString(xsResult), 16);
}

void xs_wifi419_MAC_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint8_t mac[6];

	cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifi419_SSID_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint8_t buf[36];

	if (0 == cyw43_ioctl(&cyw43_state, CYW43_IOCTL_GET_SSID, sizeof(buf), buf, CYW43_ITF_STA)) {
		uint32_t len = c_read32(buf);
		if (len && (len <= 32))
			xsmcSetStringBuffer(xsResult, (char *)(buf + sizeof(uint32_t)), len);
	}	
}

void xs_wifi419_BSSID_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint8_t bssid[6];

	if (0 == cyw43_wifi_get_bssid(&cyw43_state, bssid)) {
		xsResult = xsStringBuffer(NULL, 17);
		formatMAC(bssid, xsmcToString(xsResult));
	}
}

void xs_wifi419_RSSI_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	int32_t rssi;

	if (0 == cyw43_wifi_get_rssi(&cyw43_state, &rssi))
		xsmcSetInteger(xsResult, rssi);
}

void xs_wifi419_channel_get(xsMachine *the)
{
	(void)xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint32_t channel;

	if (0 == cyw43_ioctl(&cyw43_state, CYW43_IOCTL_GET_CHANNEL, sizeof(channel), (uint8_t *)&channel, CYW43_ITF_STA))
		xsmcSetInteger(xsResult, channel);
}
