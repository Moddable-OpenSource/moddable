/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
#include "xsmc.h"
#include "xsHost.h"

#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "esp_wifi.h"

#ifndef MODDEF_WIFI_ESP32_CONNECT_RETRIES
	#define MODDEF_WIFI_ESP32_CONNECT_RETRIES (5)
#endif

int8_t gWiFiState = -2;	// -2 = uninitialized, -1 - wifi task uninitialized, 0 = not started, 1 = starting, 2 = started, 3 = connecting, 4 = connected, 5 = IP address
int8_t gDisconnectReason = 0;		// -1 = password rejected
int8_t gWiFiConnectRetryRemaining;
int8_t gWiFiIP;		// 0x01 == IP4, 0x02 == IP6

static void initWiFi(int mode);

struct wifiScanRecord {
	xsSlot			callback;
	xsMachine		*the;
	int				canceled;
};
typedef struct wifiScanRecord wifiScanRecord;

static wifiScanRecord *gScan;
static esp_netif_t *gStation;
static esp_netif_t *gAP;

void xs_wifi_set_mode(xsMachine *the)
{
	int mode = xsmcToInteger(xsArg(0));

	initWiFi(mode);

	if (1 == mode)
		esp_wifi_set_mode(WIFI_MODE_STA);
	else if (2 == mode)
		esp_wifi_set_mode(WIFI_MODE_AP);
	else if (3 == mode)
		esp_wifi_set_mode(WIFI_MODE_APSTA);
	else if (0 == mode)
		esp_wifi_set_mode(WIFI_MODE_NULL);
	else
		xsUnknownError("invalid mode");
}

void xs_wifi_get_mode(xsMachine *the)
{
	wifi_mode_t mode;

	initWiFi(WIFI_MODE_NULL);

	esp_wifi_get_mode(&mode);
	if (WIFI_MODE_STA == mode)
		xsmcSetInteger(xsResult, 1);
	else if (WIFI_MODE_AP == mode)
		xsmcSetInteger(xsResult, 2);
	else if (WIFI_MODE_APSTA == mode)
		xsmcSetInteger(xsResult, 3);
	else if (WIFI_MODE_NULL == mode)
		xsmcSetInteger(xsResult, 0);
}

void xs_wifi_scan(xsMachine *the)
{
	wifi_scan_config_t config = {0};

	initWiFi(WIFI_MODE_STA);

	if (0 == xsmcArgc) {
		// clear gScan first because SYSTEM_EVENT_SCAN_DONE is triggered by esp_wifi_scan_stop
		if (gScan) {
			if (gScan->the != the)
				xsUnknownError("can't stop scan started by another VM");
			xsForget(gScan->callback);
			c_free(gScan);
			gScan = NULL;

			esp_wifi_scan_stop();
		}
		return;
	}

	if (gScan)
		xsUnknownError("already scanning");

	if ((xsmcArgc < 2) || (xsReferenceType != xsmcTypeOf(xsArg(0))))
		xsUnknownError("invalid");

	config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_hidden)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
		config.show_hidden = xsmcTest(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		config.channel = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_active)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_active);
		config.scan_type = xsmcTest(xsVar(0)) ? WIFI_SCAN_TYPE_ACTIVE : WIFI_SCAN_TYPE_PASSIVE;
	}

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
	gScan->the = the;

	if (ESP_OK != esp_wifi_scan_start(&config, 0)) {
		c_free(gScan);
		gScan = NULL;
		xsUnknownError("scan request failed");
	}

	gScan->callback = xsArg(1);
	xsRemember(gScan->callback);
}

void xs_wifi_connect(xsMachine *the)
{
	wifi_config_t config;
	char *str;
	int argc = xsmcArgc;
	wifi_mode_t mode;
	int channel;

	if (gWiFiState > 1) {
		gWiFiState = 2;
		gDisconnectReason = 0;
		esp_wifi_disconnect();
	}

	if (0 == argc)
		return;

	initWiFi(WIFI_MODE_STA);

	c_memset(&config, 0, sizeof(config));

	config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

	xsmcVars(2);
	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	if (!xsmcTest(xsVar(0)))
		xsUnknownError("ssid required");
	str = xsmcToString(xsVar(0));
	if (espStrLen(str) > sizeof(config.sta.ssid))
		xsUnknownError("ssid too long - 32 bytes max");
	espMemCpy(config.sta.ssid, str, espStrLen(str));

	xsmcGet(xsVar(0), xsArg(0), xsID_password);
	if (xsmcTest(xsVar(0))) {
		str = xsmcToString(xsVar(0));
		if (espStrLen(str) > sizeof(config.sta.password))
			xsUnknownError("password too long - 64 bytes max");
		espMemCpy(config.sta.password, str, espStrLen(str));
	}

	if (xsmcHas(xsArg(0), xsID_bssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bssid);
		if (sizeof(config.sta.bssid) != xsmcGetArrayBufferLength(xsVar(0)))
			xsUnknownError("bssid must be 6 bytes");
		xsmcGetArrayBufferData(xsVar(0), 0, config.sta.bssid, sizeof(config.sta.bssid));
		config.sta.bssid_set = 1;
	}

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsUnknownError("invalid channel");
		config.sta.channel = channel;
	}

	//@@ does this need to be different for WIFI_MODE_APSTA?
	esp_wifi_set_config(WIFI_IF_STA, &config); 

	gWiFiConnectRetryRemaining = MODDEF_WIFI_ESP32_CONNECT_RETRIES;
	if (0 != esp_wifi_connect())
		xsUnknownError("esp_wifi_connect failed");
}

static void reportScan(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint16_t count, i;
	wifi_ap_record_t *aps = NULL;
	wifiScanRecord *scan = gScan;

	gScan = NULL;

	xsBeginHost(the);

	if (!scan || scan->canceled)
		goto done;

	xsmcVars(2);
	esp_wifi_scan_get_ap_num(&count);

	aps = c_malloc(sizeof(wifi_ap_record_t) * count);
	if (!aps) goto done;

	esp_wifi_scan_get_ap_records(&count, aps);

	for (i = 0; i < count; i++) {
		wifi_ap_record_t *bss = &aps[i];

		xsTry {
			xsmcSetNewObject(xsVar(1));

			xsmcSetString(xsVar(0), bss->ssid);
			xsmcSet(xsVar(1), xsID_ssid, xsVar(0));

			xsmcSetInteger(xsVar(0), bss->rssi);
			xsmcSet(xsVar(1), xsID_rssi, xsVar(0));

			xsmcSetInteger(xsVar(0), bss->primary);
			xsmcSet(xsVar(1), xsID_channel, xsVar(0));

			xsmcSetBoolean(xsVar(0), 0);
			xsmcSet(xsVar(1), xsID_hidden, xsVar(0));

			xsmcSetArrayBuffer(xsVar(0), bss->bssid, sizeof(bss->bssid));
			xsmcSet(xsVar(1), xsID_bssid, xsVar(0));

			if (bss->authmode < WIFI_AUTH_MAX) {
				if (WIFI_AUTH_OPEN == bss->authmode)
					xsmcSetString(xsVar(0), "none");
				else if (WIFI_AUTH_WEP == bss->authmode)
					xsmcSetString(xsVar(0), "wep");
				else if (WIFI_AUTH_WPA_PSK == bss->authmode)
					xsmcSetString(xsVar(0), "wpa_psk");
				else if (WIFI_AUTH_WPA2_PSK == bss->authmode)
					xsmcSetString(xsVar(0), "wpa2_psk");
				else // if (WIFI_AUTH_WPA_WPA2_PSK == bss->authmode)
					xsmcSetString(xsVar(0), "wpa_wpa2_psk");

				xsmcSet(xsVar(1), xsID_authentication, xsVar(0));
			}

			xsCallFunction1(scan->callback, xsGlobal, xsVar(1));
		}
		xsCatch {
		}
	}

done:
	if (scan)
		xsCallFunction1(scan->callback, xsGlobal, xsNull);		// end of scan

	xsEndHost(the);

	if (scan)
		xsForget(scan->callback);
bail:
	if (aps)
		c_free(aps);
}

typedef struct xsWiFiRecord xsWiFiRecord;
typedef xsWiFiRecord *xsWiFi;

struct xsWiFiRecord {
	xsWiFi				next;
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				haveCallback;
};

static xsWiFi gWiFi;

static void wifiEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
    xsWiFi wifi = refcon;
    int32_t event_id = *(int32_t *)message;
    const char *msg;

    switch (event_id) {
        case WIFI_EVENT_STA_START:          msg = "start"; break;
        case WIFI_EVENT_STA_CONNECTED:      msg = "connect"; break;
        case WIFI_EVENT_STA_DISCONNECTED:   msg = "disconnect"; break;
		case WIFI_EVENT_AP_STACONNECTED:	msg = "station_connect"; break;
		case WIFI_EVENT_AP_STADISCONNECTED: msg = "station_disconnect"; break;
        default: return;
    }

    xsBeginHost(the);
        if (WIFI_EVENT_STA_DISCONNECTED != event_id)
            xsCall1(wifi->obj, xsID_callback, xsString(msg));
        else {
            xsmcSetInteger(xsResult, gDisconnectReason);
            xsCall2(wifi->obj, xsID_callback, xsString(msg), xsResult);
        }
    xsEndHost(the);
}

static void ipEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
    xsWiFi wifi = refcon;
    int32_t event_id = *(int32_t *)message;
    const char *msg;

    switch (event_id) {
        case IP_EVENT_STA_GOT_IP:            msg = "gotIP"; break;
//        case IP_EVENT_STA_CHANGED_IP:        msg = "changedIP"; break;
        default: return;
    }

    xsBeginHost(the);
        xsCall1(wifi->obj, xsID_callback, xsString(msg));
    xsEndHost(the);
}

void xs_wifi_destructor(void *data)
{
	xsWiFi wifi = data;

	if (wifi) {
		if (wifi == gWiFi)
			gWiFi = wifi->next;
		else {
			xsWiFi walker;
			for (walker = gWiFi; walker->next != wifi; walker = walker->next)
				;
			walker->next = wifi->next;
		}

		c_free(wifi);
	}
}

void xs_wifi_constructor(xsMachine *the)
{
	int argc = xsmcArgc;

	if (1 == argc)
		xsCall1(xsThis, xsID_build, xsArg(0));
	else if (2 == argc)
		xsCall2(xsThis, xsID_build, xsArg(0), xsArg(1));
}

void xs_wifi_close(xsMachine *the)
{
	xsWiFi wifi = xsmcGetHostData(xsThis);
	if (wifi) {
		if (wifi->haveCallback)
			xsForget(wifi->obj);
	}
	xs_wifi_destructor(wifi);
	xsmcSetHostData(xsThis, NULL);
}

void xs_wifi_set_onNotify(xsMachine *the)
{
	xsWiFi wifi = xsmcGetHostData(xsThis);

	if (NULL == wifi) {
		wifi = c_calloc(1, sizeof(xsWiFiRecord));
		if (!wifi)
			xsUnknownError("out of memory");
		xsmcSetHostData(xsThis, wifi);
		wifi->the = the;
		wifi->obj = xsThis;
	}
	else if (wifi->haveCallback) {
		xsmcDelete(xsThis, xsID_callback);
		wifi->haveCallback = false;
		xsForget(wifi->obj);
	}

	if (!xsmcTest(xsArg(0)))
		return;

	wifi->haveCallback = true;

	xsRemember(wifi->obj);

	wifi->next = gWiFi;
	gWiFi = wifi;

	xsmcSet(xsThis, xsID_callback, xsArg(0));
}

static void doWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	wifi_config_t wifi_config;
	xsWiFi walker;

	switch (event_id) {
		case WIFI_EVENT_STA_START:
			if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &wifi_config)) {
#ifdef MODDEF_WIFI_HOSTNAME
                esp_netif_set_hostname(gStation, MODDEF_WIFI_HOSTNAME);
#endif

				gWiFiState = 3;
				gWiFiConnectRetryRemaining = MODDEF_WIFI_ESP32_CONNECT_RETRIES;
				esp_wifi_connect();
			}
			else
				gWiFiState = 2;
			break;
		case WIFI_EVENT_STA_STOP:
			gWiFiState = 1;
			if (gScan) {
				gScan->canceled = 1;
				modMessagePostToMachine(gScan->the, NULL, 0, reportScan, NULL);
			}
			break;
		case WIFI_EVENT_STA_CONNECTED:
			gWiFiState = 4;
			gWiFiConnectRetryRemaining = 0;
			gWiFiIP = 0;
            if (ESP_OK != esp_netif_create_ip6_linklocal(gStation))
				gWiFiIP = 0x02;		// don't wait for IP6 address if esp_netif_create_ip6_linklocal failed
			break;
		case WIFI_EVENT_STA_DISCONNECTED: {
			wifi_err_reason_t reason = ((wifi_event_sta_disconnected_t *)event_data)->reason;
			gWiFiState = 2;
			gWiFiIP = 0;
			gDisconnectReason =	((WIFI_REASON_MIC_FAILURE == reason) ||
								(WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT == reason) ||
								(WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT == reason) ||
								(WIFI_REASON_IE_IN_4WAY_DIFFERS == reason) ||
								(WIFI_REASON_HANDSHAKE_TIMEOUT == reason)) ? -1 : gDisconnectReason;
			if (gWiFiConnectRetryRemaining > 0) {
				if (0 == esp_wifi_connect()) {
					gWiFiConnectRetryRemaining -= 1;
					return;
				}
				gWiFiConnectRetryRemaining = 0;
			}
			} break;
		case WIFI_EVENT_SCAN_DONE:
			if (gScan)
				modMessagePostToMachine(gScan->the, NULL, 0, reportScan, NULL);
            return;
		case WIFI_EVENT_AP_STACONNECTED:
		case WIFI_EVENT_AP_STADISCONNECTED:
			break;
		default:
            return;
	}

	for (walker = gWiFi; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), wifiEventPending, walker);
}

static void doIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    wifi_config_t wifi_config;
    xsWiFi walker;

    switch (event_id) {
        case IP_EVENT_GOT_IP6:
            if (0x03 == gWiFiIP)
                return;
            gWiFiIP |= 0x02;
            if (0x03 != gWiFiIP)
                return;
            event_id = IP_EVENT_STA_GOT_IP;
            gWiFiState = 5;
            break;

        case IP_EVENT_STA_GOT_IP:
            gWiFiIP |= 0x01;
            if (0x03 != gWiFiIP)
                return;
//@@            if (((ip_event_got_ip_t *)event_data)->ip_changed && (5 == gWiFiState))        // N.B. ip_changed is set when initial IP address received.
//@@                event_id = IP_EVENT_STA_CHANGED_IP;
            gWiFiState = 5;
            break;
    }

    for (walker = gWiFi; NULL != walker; walker = walker->next)
        modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), ipEventPending, walker);
}

void initWiFi(int mode)
{
	if (gWiFiState > 0) return;

	if (gWiFiState <= -2) {
        esp_netif_init();
		esp_err_t err = esp_event_loop_create_default();
		if (ESP_ERR_INVALID_STATE != err)		// ESP_ERR_INVALID_STATE indicates the default event loop has already been created
			ESP_ERROR_CHECK(err);
	}

	gWiFiState = 1;

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;		// we manage the Wi-Fi connection. don't want surprises from what may be in NVS.
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	esp_wifi_set_mode(mode);

	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    gStation = esp_netif_create_default_wifi_sta();

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, doWiFiEvent, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, doIPEvent, NULL, NULL));
    
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void xs_wifi_accessPoint(xsMachine *the)
{
	wifi_mode_t mode;
	wifi_config_t config;
	wifi_ap_config_t *ap;
    esp_netif_ip_info_t info;
	char *str;
	uint8_t station = 0;

	initWiFi(WIFI_MODE_AP);
	
	c_memset(&config, 0, sizeof(config));
	ap = &config.ap;

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	str = xsmcToString(xsVar(0));
	ap->ssid_len = c_strlen(str);
	if (ap->ssid_len > sizeof(ap->ssid))
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(ap->ssid, str, ap->ssid_len);

	ap->authmode = WIFI_AUTH_OPEN;
	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > sizeof(ap->password))
			xsUnknownError("password too long - 64 bytes max");
		if (c_strlen(str) < 8)
			xsUnknownError("password too short - 8 bytes min");
		if (!c_isEmpty(str)) {
			c_memcpy(ap->password, str, c_strlen(str));
			ap->authmode = WIFI_AUTH_WPA_WPA2_PSK;
		}
	}

	ap->channel = 1;
	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		ap->channel = xsmcToInteger(xsVar(0));
		if ((ap->channel < 1) || (ap->channel > 13))
			xsUnknownError("invalid channel");
	}

	ap->ssid_hidden = 0;
	if (xsmcHas(xsArg(0), xsID_hidden)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
		ap->ssid_hidden = xsmcTest(xsVar(0));
	}

	ap->max_connection = 4;
	if (xsmcHas(xsArg(0), xsID_max)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_max);
		ap->max_connection = xsmcToInteger(xsVar(0));
	}

	ap->beacon_interval = 100;
	if (xsmcHas(xsArg(0), xsID_interval)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_interval);
		ap->beacon_interval = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_station)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_station);
		station = xsmcToBoolean(xsVar(0));
	}

    if (!gAP)
        gAP = esp_netif_create_default_wifi_ap();

    esp_wifi_get_mode(&mode);
	if ((WIFI_MODE_AP != mode) && (WIFI_MODE_APSTA != mode)) {
		if (ESP_OK != esp_wifi_set_mode(station ? WIFI_MODE_APSTA : WIFI_MODE_AP))
			xsUnknownError("esp_wifi_set_mode failed");
	}

    if (ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_AP, &config))
		xsUnknownError("esp_wifi_set_config failed");

    if (ESP_OK != esp_wifi_start())
		xsUnknownError("esp_wifi_start failed");
	if (ESP_OK != esp_netif_get_ip_info(gAP, &info))
		xsUnknownError("esp_netif_get_ip_info failed");
	if (0 == info.ip.addr)
		xsUnknownError("IP config bad when starting Wi-Fi AP!");
}
