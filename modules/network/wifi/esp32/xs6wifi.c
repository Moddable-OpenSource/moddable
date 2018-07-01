/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "xsesp.h"

#include "mc.xs.h"			// for xsID_ values

#include "esp_wifi.h"

// maybe use task event group flags for this like all the samples?
int8_t gWiFiState = -1;	// -1 = uninitialized, 0 = not started, 1 = starting, 2 = started, 3 = connecting, 4 = connected, 5 = IP address

static void initWiFi(void);

struct wifiScanRecord {
	xsSlot			callback;
	xsMachine		*the;
};
typedef struct wifiScanRecord wifiScanRecord;

static wifiScanRecord *gScan;

void xs_wifi_set_mode(xsMachine *the)
{
	int mode = xsmcToInteger(xsArg(0));

	initWiFi();

	if (1 == mode)
		esp_wifi_set_mode(WIFI_MODE_STA);
	else if (2 == mode)
		esp_wifi_set_mode(WIFI_MODE_AP);
	else
		xsUnknownError("invalid mode");
}

void xs_wifi_get_mode(xsMachine *the)
{
	wifi_mode_t mode;

	initWiFi();

	esp_wifi_get_mode(&mode);
	if (WIFI_MODE_STA == mode)
		xsmcSetInteger(xsResult, 1);
	else if (WIFI_MODE_AP == mode)
		xsmcSetInteger(xsResult, 2);
}

void xs_wifi_scan(xsMachine *the)
{
	wifi_mode_t mode;
	wifi_scan_config_t config;

	initWiFi();

	esp_wifi_get_mode(&mode);
	if (WIFI_MODE_STA != mode)
		xsUnknownError("can't scan in WIFI_MODE_STA");

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

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
	gScan->callback = xsArg(1);
	gScan->the = the;
	xsRemember(gScan->callback);

	config.ssid = NULL;
	config.bssid = NULL;
	config.channel = 0;
	config.show_hidden = 0;
	config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
	config.scan_time.active.min = config.scan_time.active.max = 0;

	if (xsmcArgc) {
		xsmcVars(1);

		if (xsmcHas(xsArg(0), xsID_hidden)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
			config.show_hidden = xsmcTest(xsVar(0));
		}

		if (xsmcHas(xsArg(0), xsID_channel)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_channel);
			config.channel = xsmcToInteger(xsVar(0));
		}
	}

	if (ESP_OK != esp_wifi_scan_start(&config, 0)) {
		xsForget(gScan->callback);
		c_free(gScan);
		gScan = NULL;
		xsUnknownError("scan request failed");
	}
}

void xs_wifi_status(xsMachine *the)
{
//@@
//	station_status_t status = wifi_station_get_connect_status();
//
//	xsmcSetInteger(xsResult, (int)status);
}

void xs_wifi_connect(xsMachine *the)
{
	wifi_config_t config;
	char *str;
	int argc = xsmcToInteger(xsArgc);
	wifi_mode_t mode;

	initWiFi();

	if (gWiFiState >= 3)
		esp_wifi_disconnect();

	if (0 == argc)
		return;

	c_memset(&config, 0, sizeof(config));

	xsmcVars(2);
	if (xsmcHas(xsArg(0), xsID_ssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
		str = xsmcToString(xsVar(0));
		if (espStrLen(str) > (sizeof(config.sta.ssid) - 1))
			xsUnknownError("ssid too long - 32 bytes max");
		espMemCpy(config.sta.ssid, str, espStrLen(str));
	}

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (espStrLen(str) > (sizeof(config.sta.password) - 1))
			xsUnknownError("password too long - 64 bytes max");
		espMemCpy(config.sta.password, str, espStrLen(str));
	}

	if (xsmcHas(xsArg(0), xsID_bssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bssid);
		if (sizeof(config.sta.bssid) != xsGetArrayBufferLength(xsVar(0)))
			xsUnknownError("bssid must be 6 bytes");
		xsmcGetArrayBufferData(xsVar(0), 0, config.sta.bssid, sizeof(config.sta.bssid));
		config.sta.bssid_set = 1;
	}

	esp_wifi_get_mode(&mode);
	if (WIFI_MODE_STA != mode)
		esp_wifi_set_mode(WIFI_MODE_STA);

	if (gWiFiState >= 4)
		esp_wifi_disconnect();

	esp_wifi_set_config(WIFI_IF_STA, &config);

	if (0 != esp_wifi_connect())
		xsUnknownError("esp_wifi_connect failed");
}

static void reportScan(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint16_t count, i;
	wifi_ap_record_t *aps;
	wifiScanRecord *scan = gScan;

	gScan = NULL;

	esp_wifi_scan_get_ap_num(&count);

	aps = malloc(sizeof(wifi_ap_record_t) * count);
	esp_wifi_scan_get_ap_records(&count, aps);

	xsBeginHost(the);

	xsmcVars(2);
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

	xsCallFunction1(scan->callback, xsGlobal, xsNull);		// end of scan

	xsEndHost(the);

	xsForget(scan->callback);
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
	system_event_id_t event_id = *(system_event_id_t *)message;
	const char *msg;

	switch (event_id){
		case SYSTEM_EVENT_STA_START:			msg = "start"; break;
		case SYSTEM_EVENT_STA_CONNECTED:		msg = "connect"; break;
		case SYSTEM_EVENT_STA_DISCONNECTED:		msg = "disconnect"; break;
		case SYSTEM_EVENT_STA_GOT_IP:			msg = "gotIP"; break;
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
	int argc = xsmcToInteger(xsArgc);

	if (1 == argc)
		xsCall1(xsThis, xsID_build, xsArg(0));
	else if (2 == argc)
		xsCall2(xsThis, xsID_build, xsArg(0), xsArg(1));
}

void xs_wifi_close(xsMachine *the)
{
	xsWiFi wifi = xsmcGetHostData(xsThis);
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
	}
	else if (wifi->haveCallback) {
		xsmcDelete(xsThis, xsID_callback);
		wifi->haveCallback = false;
	}

	if (!xsmcTest(xsArg(0)))
		return;

	wifi->haveCallback = true;

	wifi->next = gWiFi;
	gWiFi = wifi;

	wifi->obj = xsThis;
	xsmcSet(xsThis, xsID_callback, xsArg(0));

	xsCall1(wifi->obj, xsID_callback, xsString("init"));		//@@ this should be unnecessary
}

static esp_err_t doWiFiEvent(void *ctx, system_event_t *event)
{
	wifi_config_t wifi_config;
	xsWiFi walker;

    switch (event->event_id) {
		case SYSTEM_EVENT_STA_START:
			if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &wifi_config)) {
				gWiFiState = 3;
				esp_wifi_connect();
			}
			else
				gWiFiState = 2;
			break;
		case SYSTEM_EVENT_STA_CONNECTED:
			gWiFiState = 4;
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			gWiFiState = 5;
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			gWiFiState = 2;
			break;
		case SYSTEM_EVENT_SCAN_DONE:
			if (gScan)
				modMessagePostToMachine(gScan->the, (uint8_t *)&event->event_id, sizeof(event->event_id), reportScan, NULL);
			return ESP_OK;
		default:
			return ESP_OK;
	}

	for (walker = gWiFi; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, (uint8_t *)&event->event_id, sizeof(event->event_id), wifiEventPending, walker);

	return ESP_OK;
}

void initWiFi(void)
{
	if (gWiFiState > 0) return;

	if (-1 == gWiFiState) {
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		tcpip_adapter_init();
		ESP_ERROR_CHECK( esp_event_loop_init(NULL, NULL) );
		ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
		gWiFiState = 0;
	}

	ESP_ERROR_CHECK( esp_event_loop_set_cb(doWiFiEvent, NULL) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_start() );

	gWiFiState = 1;
}

void xs_wifi_accessPoint(xsMachine *the)
{
    wifi_mode_t mode;
	wifi_config_t config;
	wifi_ap_config_t *ap;
	tcpip_adapter_ip_info_t info;
	char *str;

	initWiFi();
	
	c_memset(&config, 0, sizeof(config));
	ap = &config.ap;
	
	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	str = xsmcToString(xsVar(0));
	ap->ssid_len = c_strlen(str);
	if (ap->ssid_len > (sizeof(ap->ssid) - 1))
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(ap->ssid, str, ap->ssid_len);

	ap->authmode = WIFI_AUTH_OPEN;
	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > (sizeof(ap->password) - 1))
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

	esp_wifi_get_mode(&mode);
	if (WIFI_MODE_AP != mode) {
		if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_AP))
			xsUnknownError("esp_wifi_set_mode failed");
	}

	if (ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_AP, &config))
		xsUnknownError("esp_wifi_set_config failed");
	if (ESP_OK != esp_wifi_start())
		xsUnknownError("esp_wifi_start failed");
	if (ESP_OK != tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &info))
		xsUnknownError("tcpip_adapter_get_ip_info failed");
	if (0 == info.ip.addr)
        xsUnknownError("IP config bad when starting Wi-Fi AP!\n");
}

