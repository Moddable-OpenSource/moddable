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

#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/ip6_addr.h"

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
	uint8_t		ip;

	xsSlot		*onChanged;
	xsSlot		*onFound;
	xsSlot		*onComplete;
} xsWiFiRecord;

static esp_netif_t *gStation;
static xsWiFi gWiFiList;
static xsWiFi gScan;
static uint8_t gIP;

static void doWiFiEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void doIPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
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

static const char *authmodeToString(wifi_auth_mode_t authmode)
{
	static const char *const modes[] = {
		"none", "wep", "wpa_psk", "wpa2_psk", "wpa_wpa2_psk",
		"wpa2_enterprise", "wpa3_psk", "wpa2_wpa3_psk"
	};
	if (authmode < 8)
		return modes[authmode];
	return "unknown";
}

typedef struct {
	int32_t		event_id;
	uint8_t		ipInit;
} WiFiEventMsg;

void xs_wifi419_destructor(void *data)
{
	xsWiFi wf = (xsWiFi)data;
	if (!wf) return;

	if (__atomic_sub_fetch(&wf->useCount, 1, __ATOMIC_SEQ_CST) > 0)
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
	__atomic_store_n(&wf->useCount, 1, __ATOMIC_SEQ_CST);
	wf->onChanged = onChanged;

	xsmcSetHostData(xsThis, wf);
	xsSetHostHooks(xsThis, &xsWiFiHooks);
	xsRemember(wf->obj);

	initWiFi();

	wifi_ap_record_t ap_info;
	if (ESP_OK == esp_wifi_sta_get_ap_info(&ap_info)) {
		wf->connected = 1;

		esp_netif_ip_info_t ip_info;
		if (ESP_OK == esp_netif_get_ip_info(gStation, &ip_info) && ip_info.ip.addr)
			wf->ip |= 0x01;

		esp_ip6_addr_t ip6;
		if (ESP_OK == esp_netif_get_ip6_linklocal(gStation, &ip6))
			wf->ip |= 0x02;

		gIP = wf->ip;
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

	if (gScan == wf) {
		esp_wifi_scan_stop();
		gScan = C_NULL;
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
	wifi_mode_t mode;
	if (ESP_OK == esp_wifi_get_mode(&mode)) {
		if (!gStation)
			gStation = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
		return;
	}

	if (gStation) return;

	esp_netif_init();
	esp_err_t err = esp_event_loop_create_default();
	if (ESP_ERR_INVALID_STATE != err)
		ESP_ERROR_CHECK(err);

	gStation = esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, doWiFiEvent, C_NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, doIPEvent, C_NULL));

	ESP_ERROR_CHECK(esp_wifi_start());
}

static void doWiFiEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	xsWiFi walker;

	if ((WIFI_EVENT_SCAN_DONE == event_id) || (WIFI_EVENT_STA_STOP == event_id)) {
		xsWiFi scan = gScan;
		if (scan) {
			gScan = C_NULL;
			__atomic_add_fetch(&scan->useCount, 1, __ATOMIC_SEQ_CST);
			modMessagePostToMachine(scan->the, C_NULL, 0, wifiScanDeliver, scan);
		}
		return;
	}

	WiFiEventMsg msg;
	msg.ipInit = 0;

	if (WIFI_EVENT_STA_CONNECTED == event_id) {
		gIP = 0;
		if (ESP_OK != esp_netif_create_ip6_linklocal(gStation))
			gIP = 0x02;
		msg.ipInit = gIP;
	}
	else if (WIFI_EVENT_STA_DISCONNECTED == event_id) {
		gIP = 0;
	}
	else
		return;

	msg.event_id = event_id;
	for (walker = gWiFiList; walker; walker = walker->next) {
		__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachine(walker->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, walker);
	}
}

static void doIPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	xsWiFi walker;

	if (IP_EVENT_GOT_IP6 == event_id) {
		if (0x03 == gIP) return;
		gIP |= 0x02;
		if (0x03 != gIP) return;
		event_id = IP_EVENT_STA_GOT_IP;
	}
	else if (IP_EVENT_STA_GOT_IP == event_id) {
		gIP |= 0x01;
		if (0x03 != gIP) return;
	}
	else
		return;

	WiFiEventMsg msg;
	msg.event_id = event_id;
	msg.ipInit = 0;
	for (walker = gWiFiList; walker; walker = walker->next) {
		__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachine(walker->the, (uint8_t *)&msg, sizeof(msg), wifiConnectDeliver, walker);
	}
}

static void wifiScanDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsWiFi wf = refcon;

	if (wf->closed || !wf->scanning)
		goto bail;

	wf->scanning = 0;

	xsBeginHost(the);

	wifi_ap_record_t bss;
	xsmcVars(1);
	while (ESP_OK == esp_wifi_scan_get_ap_record(&bss)) {
		xsTry {
			xsmcSetNewObject(xsResult);

			xsmcSetString(xsVar(0), (char *)bss.ssid);
			xsmcSet(xsResult, xsID_SSID, xsVar(0));

			xsVar(0) = xsStringBuffer(NULL, 17);
			formatMAC(bss.bssid, xsmcToString(xsVar(0)));
			xsmcSet(xsResult, xsID_BSSID, xsVar(0));

			xsmcSetInteger(xsVar(0), bss.rssi);
			xsmcSet(xsResult, xsID_RSSI, xsVar(0));

			xsmcSetInteger(xsVar(0), bss.primary);
			xsmcSet(xsResult, xsID_channel, xsVar(0));

			xsmcSetStringX(xsVar(0), (char *)authmodeToString(bss.authmode));
			xsmcSet(xsResult, xsID_security, xsVar(0));

			xsCallFunction1(xsReference(wf->onFound), wf->obj, xsResult);
		}
		xsCatch {
		}
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
	WiFiEventMsg *msg = (WiFiEventMsg *)msgIn;
	uint8_t prevConnecting = wf->connecting, prevConnected = wf->connected, prevIP = wf->ip;

	if (wf->closed)
		goto bail;

	xsBeginHost(the);

	if (WIFI_EVENT_STA_CONNECTED == msg->event_id) {
		wf->connecting = 0;
		wf->connected = 1;
		wf->ip = msg->ipInit;
	}
	else if (WIFI_EVENT_STA_DISCONNECTED == msg->event_id) {
		wf->connecting = 0;
		wf->connected = 0;
		wf->ip = 0;
	}
	else if (IP_EVENT_STA_GOT_IP == msg->event_id) {
		wf->connected = 1;
		wf->ip = 0x03;
	}

	if (wf->onChanged &&
		((prevConnecting != wf->connecting) ||
		 (prevConnected != wf->connected) ||
		 (prevIP != wf->ip)))
		xsCallFunction0(xsReference(wf->onChanged), wf->obj);

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

	wifi_scan_config_t config = {0};
	config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsSlot tmp;
		xsmcGet(tmp, xsArg(0), xsID_channel);
		config.channel = xsmcToInteger(tmp);
	}

	wf->scanning = 1;
	gScan = wf;
	if (ESP_OK != esp_wifi_scan_start(&config, 0)) {
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

	wifi_config_t config = {0};
	config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_SSID)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_SSID);
		char *ssid = xsmcToString(xsVar(0));
		if (c_strlen(ssid) > sizeof(config.sta.ssid))
			xsRangeError("SSID too long");
		c_memcpy(config.sta.ssid, ssid, c_strlen(ssid));
	}

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		char *password = xsmcToString(xsVar(0));
		if (c_strlen(password) > sizeof(config.sta.password))
			xsRangeError("password too long");
		c_memcpy(config.sta.password, password, c_strlen(password));
	}

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		int channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsRangeError("invalid channel");
		config.sta.channel = channel;
	}

	esp_wifi_set_config(WIFI_IF_STA, &config);

	wf->connecting = 1;
	if (ESP_OK != esp_wifi_connect()) {
		wf->connecting = 0;
		xsUnknownError("connect failed");
	}
}

void xs_wifi419_disconnect(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);

	esp_wifi_disconnect();

	wf->connecting = 0;
	wf->connected = 0;
	wf->ip = 0;
	gIP = 0;
}

void xs_wifi419_connection_get(xsMachine *the)
{
	xsWiFi wf = xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	int connection;

	if (wf->connected && (0x03 == wf->ip))
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

	if (!gStation) return;

	esp_netif_ip_info_t info;
	if (ESP_OK == esp_netif_get_ip_info(gStation, &info) && info.ip.addr) {
		xsResult = xsStringBuffer(NULL, 39);
		esp_ip4addr_ntoa(&info.ip, xsmcToString(xsResult), 40);
		return;
	}

	esp_ip6_addr_t ip6;
	if (ESP_OK == esp_netif_get_ip6_linklocal(gStation, &ip6)) {
		xsResult = xsStringBuffer(NULL, 39);
		ip6addr_ntoa_r((ip6_addr_t *)&ip6, xsmcToString(xsResult), 40);
	}
}

void xs_wifi419_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	uint8_t mac[6];

	if (ESP_OK != esp_wifi_get_mac(WIFI_IF_STA, mac))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifi419_SSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	wifi_ap_record_t info;

	if (ESP_OK != esp_wifi_sta_get_ap_info(&info))
		return;

	xsmcSetString(xsResult, (char *)info.ssid);
}

void xs_wifi419_BSSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	wifi_ap_record_t info;

	if (ESP_OK != esp_wifi_sta_get_ap_info(&info))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(info.bssid, xsmcToString(xsResult));
}

void xs_wifi419_RSSI_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	wifi_ap_record_t info;

	if (ESP_OK != esp_wifi_sta_get_ap_info(&info))
		return;

	xsmcSetInteger(xsResult, info.rssi);
}

void xs_wifi419_channel_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsWiFiHooks);
	wifi_ap_record_t info;

	if (ESP_OK != esp_wifi_sta_get_ap_info(&info))
		return;

	xsmcSetInteger(xsResult, info.primary);
}
