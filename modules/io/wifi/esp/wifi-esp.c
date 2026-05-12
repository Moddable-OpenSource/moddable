/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

#include "user_interface.h"

#define wifiStationAssociated() (wifi_station_get_rssi() < 10)

typedef struct {
	uint8_t		bssid[6];
	uint8_t		channel;
	int8_t		rssi;
	uint8_t		authmode;
	char			ssid[33];
} WiFiScanResult;

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

	WiFiScanResult	*scanResults;
	uint16_t	scanCount;
} xsWiFiRecord;

static xsWiFi gWiFiList;
static xsWiFi gScan;
static uint8_t gInited;

static void doWiFiEvent(System_Event_t *msg);
static void wifiScanComplete(void *arg, STATUS status);
static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void wifiConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void initWiFi(void);

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

static const char *authmodeToString(uint8_t authmode)
{
	static const char *const modes[] = {
		"none", "wep", "wpa_psk", "wpa2_psk", "wpa_wpa2_psk"
	};
	if (authmode < 5)
		return modes[authmode];
	return "unknown";
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

static const xsHostHooks ICACHE_RODATA_ATTR xsWiFiHooks = {
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

	initWiFi();

	uint8 status = wifi_station_get_connect_status();
	if (STATION_GOT_IP == status) {
		wf->connected = 1;
		wf->gotIP = 1;
	}
	else if (STATION_CONNECTING == status) {
		if (wifiStationAssociated())
			wf->connected = 1;
		else
			wf->connecting = 1;
	}

	wf->next = gWiFiList;
	gWiFiList = wf;
}

void xs_wifi419_close(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostData(xsThis);
	if (!wf) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	wf->closed = 1;
	xsForget(wf->obj);

	if (gScan == wf)
		gScan = C_NULL;

	if (wf->scanResults) {
		c_free(wf->scanResults);
		wf->scanResults = C_NULL;
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

static void initWiFi(void)
{
	if (gInited) return;
	gInited = 1;

	wifi_set_opmode_current(STATION_MODE);
	wifi_set_event_handler_cb(doWiFiEvent);
}

static void doWiFiEvent(System_Event_t *msg)
{
	xsWiFi walker;
	uint32_t event = msg->event;

	if ((EVENT_STAMODE_CONNECTED != event) &&
		(EVENT_STAMODE_DISCONNECTED != event) &&
		(EVENT_STAMODE_GOT_IP != event))
		return;

	for (walker = gWiFiList; walker; walker = walker->next) {
		walker->useCount += 1;
		modMessagePostToMachine(walker->the, (uint8_t *)&event, sizeof(event), wifiConnectDeliver, walker);
	}
}

static void wifiScanComplete(void *arg, STATUS status)
{
	xsWiFi wf = gScan;
	if (!wf) return;

	gScan = C_NULL;

	if (OK == status) {
		struct bss_info *bss;
		uint16_t count = 0;

		for (bss = arg; bss; bss = bss->next.stqe_next)
			count++;

		if (count) {
			wf->scanResults = c_malloc(sizeof(WiFiScanResult) * count);
			if (wf->scanResults) {
				WiFiScanResult *result = wf->scanResults;
				wf->scanCount = count;
				for (bss = arg; bss; bss = bss->next.stqe_next, result++) {
					c_memcpy(result->bssid, bss->bssid, 6);
					result->channel = bss->channel;
					result->rssi = bss->rssi;
					result->authmode = bss->authmode;
					uint8_t len = bss->ssid_len;
					if (len > 32) len = 32;
					c_memcpy(result->ssid, bss->ssid, len);
					result->ssid[len] = 0;
				}
			}
		}
	}

	wf->useCount += 1;
	modMessagePostToMachine(wf->the, C_NULL, 0, wifiScanDeliver, wf);
}

static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;

	if (wf->closed || !wf->scanning)
		goto bail;

	wf->scanning = 0;

	xsBeginHost(the);

	if (wf->scanResults) {
		xsmcVars(1);
		for (uint16_t i = 0; i < wf->scanCount; i++) {
			WiFiScanResult *bss = &wf->scanResults[i];

			xsTry {
				xsmcSetNewObject(xsResult);

				xsmcSetString(xsVar(0), bss->ssid);
				xsmcSet(xsResult, xsID_SSID, xsVar(0));

				xsVar(0) = xsStringBuffer(NULL, 17);
				formatMAC(bss->bssid, xsmcToString(xsVar(0)));
				xsmcSet(xsResult, xsID_BSSID, xsVar(0));

				xsmcSetInteger(xsVar(0), bss->rssi);
				xsmcSet(xsResult, xsID_RSSI, xsVar(0));

				xsmcSetInteger(xsVar(0), bss->channel);
				xsmcSet(xsResult, xsID_channel, xsVar(0));

				xsmcSetStringX(xsVar(0), (char *)authmodeToString(bss->authmode));
				xsmcSet(xsResult, xsID_security, xsVar(0));

				xsCallFunction1(xsReference(wf->onFound), wf->obj, xsResult);
			}
			xsCatch {
			}
		}
	}

	if (wf->scanResults) {
		c_free(wf->scanResults);
		wf->scanResults = C_NULL;
		wf->scanCount = 0;
	}

	if (wf->onComplete)
		xsCallFunction0(xsReference(wf->onComplete), wf->obj);

	xsEndHost(the);

bail:
	xs_wifi419_destructor(wf);
}

static void wifiConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;
	uint32_t event = *(uint32_t *)msgIn;
	uint8_t prevConnecting = wf->connecting, prevConnected = wf->connected, prevIP = wf->gotIP;

	if (wf->closed)
		goto bail;

	xsBeginHost(the);

	if (EVENT_STAMODE_CONNECTED == event) {
		wf->connecting = 0;
		wf->connected = 1;
		wf->gotIP = 0;
	}
	else if (EVENT_STAMODE_DISCONNECTED == event) {
		wf->connecting = 0;
		wf->connected = 0;
		wf->gotIP = 0;
	}
	else if (EVENT_STAMODE_GOT_IP == event) {
		wf->connected = 1;
		wf->gotIP = 1;
	}

	if (wf->onChanged &&
		((prevConnecting != wf->connecting) ||
		 (prevConnected != wf->connected) ||
		 (prevIP != wf->gotIP))) {
		xsSlot tmp;
		xsmcSetStringX(tmp, "connection");
		xsCallFunction1(xsReference(wf->onChanged), wf->obj, tmp);
	}

	xsEndHost(the);

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

	struct scan_config config = {0};
	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsSlot tmp;
		xsmcGet(tmp, xsArg(0), xsID_channel);
		config.channel = xsmcToInteger(tmp);
	}

	wf->scanning = 1;
	gScan = wf;
	if (!wifi_station_scan(&config, wifiScanComplete)) {
		wf->scanning = 0;
		gScan = C_NULL;
		xsUnknownError("scan failed");
	}
}

void xs_wifi419_connect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wf->connecting)
		xsUnknownError("already connecting");

	struct station_config config = {0};

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_SSID)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_SSID);
		char *ssid = xsmcToString(xsVar(0));
		if (c_strlen(ssid) > sizeof(config.ssid))
			xsRangeError("SSID too long");
		c_memcpy(config.ssid, ssid, c_strlen(ssid));
	}

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		char *password = xsmcToString(xsVar(0));
		if (c_strlen(password) > sizeof(config.password))
			xsRangeError("password too long");
		c_memcpy(config.password, password, c_strlen(password));
	}

	wifi_station_set_config_current(&config);

	wifi_set_sleep_type(NONE_SLEEP_T);

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		int channel = xsmcToInteger(xsVar(0));
		if ((channel >= 1) && (channel <= 13))
			wifi_set_channel(channel);
	}

	wf->connecting = 1;
	wifi_station_connect();
}

void xs_wifi419_disconnect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	wifi_station_disconnect();

	wf->connecting = 0;
	wf->connected = 0;
	wf->gotIP = 0;
}

void xs_wifi419_connection_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	int connection;

	if (wf->connected && wf->gotIP)
		connection = 500;
	else if (wf->connected)
		connection = 400;
	else if (wf->connecting)
		connection = 300;
	else
		connection = 200;

	xsmcSetInteger(xsResult, connection);
}

void xs_wifi419_address_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	struct ip_info info;
	if (!wifi_get_ip_info(STATION_IF, &info))
		return;

	if (!info.ip.addr)
		return;

	xsResult = xsStringBuffer(NULL, 15);
	ipaddr_ntoa_r(&info.ip, xsmcToString(xsResult), 16);
}

void xs_wifi419_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint8_t mac[6];

	if (!wifi_get_macaddr(STATION_IF, mac))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifi419_SSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (!wifiStationAssociated())
		return;

	struct station_config config;
	if (wifi_station_get_config(&config) && config.ssid[0])
		xsmcSetString(xsResult, (char *)config.ssid);
}

void xs_wifi419_BSSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (!wifiStationAssociated())
		return;

	struct station_config config;
	if (wifi_station_get_config(&config)) {
		xsResult = xsStringBuffer(NULL, 17);
		formatMAC(config.bssid, xsmcToString(xsResult));
	}
}

void xs_wifi419_RSSI_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wifiStationAssociated())
		xsmcSetInteger(xsResult, wifi_station_get_rssi());
}

void xs_wifi419_channel_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	if (wifiStationAssociated())
		xsmcSetInteger(xsResult, wifi_get_channel());
}
