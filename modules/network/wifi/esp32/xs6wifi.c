/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "string.h"
#include "malloc.h"

#include "esp_wifi.h"

// maybe use task event group flags for this like all the samples?
uint8_t gWiFiState = 0;	// 0 = not started, 1 = starting, 2 = started, 3 = connecting, 4 = connected, 5 = IP address

static void initWiFi(void);

static void reportScan(void);

struct wifiScanRecord {
	xsSlot			callback;
	struct bss_info	*scan;
	void			*data;
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

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
	gScan->callback = xsArg(1);
	xsRemember(gScan->callback);

	config.ssid = NULL;
	config.bssid = NULL;
	config.channel = 0;
	config.show_hidden = 0;
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

	initWiFi();

	if (gWiFiState >= 3)
		esp_wifi_disconnect();

	if (0 == argc)
		return;

	c_memset(&config, 0, sizeof(config));

	xsmcVars(2);
	if (xsmcHas(xsArg(0), xsID("ssid"))) {
		xsmcGet(xsVar(0), xsArg(0), xsID("ssid"));
		str = xsmcToString(xsVar(0));
		if (espStrLen(str) > (sizeof(config.sta.ssid) - 1))
			xsUnknownError("ssid too long - 32 bytes max");
		espMemCpy(config.sta.ssid, str, espStrLen(str));
	}

	if (xsmcHas(xsArg(0), xsID("password"))) {
		xsmcGet(xsVar(0), xsArg(0), xsID("password"));
		str = xsmcToString(xsVar(0));
		if (espStrLen(str) > (sizeof(config.sta.password) - 1))
			xsUnknownError("password too long - 64 bytes max");
		espMemCpy(config.sta.password, str, espStrLen(str));
	}

	if (xsmcHas(xsArg(0), xsID("bssid"))) {
		xsmcGet(xsVar(0), xsArg(0), xsID("bssid"));
		if (sizeof(config.sta.bssid) != xsGetArrayBufferLength(xsVar(0)))
			xsUnknownError("bssid must be 6 bytes");
		xsmcGetArrayBufferData(xsVar(0), 0, config.sta.bssid, sizeof(config.sta.bssid));
		config.sta.bssid_set = 1;
	}

	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_wifi_set_config(WIFI_IF_STA, &config);

	if (gWiFiState >= 3)
		esp_wifi_connect();
}

void reportScan(void)
{
	xsMachine *the = gThe;
	uint16_t count, i;
	wifi_ap_record_t *aps;

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
			xsmcSet(xsVar(1), xsID("ssid"), xsVar(0));

			xsmcSetInteger(xsVar(0), bss->rssi);
			xsmcSet(xsVar(1), xsID("rssi"), xsVar(0));

			xsmcSetInteger(xsVar(0), bss->primary);
			xsmcSet(xsVar(1), xsID("channel"), xsVar(0));

			xsmcSetBoolean(xsVar(0), 0);
			xsmcSet(xsVar(1), xsID("hidden"), xsVar(0));

			xsmcSetArrayBuffer(xsVar(0), bss->bssid, sizeof(bss->bssid));
			xsmcSet(xsVar(1), xsID("bssid"), xsVar(0));

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

				xsmcSet(xsVar(1), xsID("authentication"), xsVar(0));
			}

			xsCallFunction1(gScan->callback, xsGlobal, xsVar(1));
		}
		xsCatch {
		}
	}

	xsCallFunction1(gScan->callback, xsGlobal, xsNull);		// end of scan

	xsEndHost(the);

	xsForget(gScan->callback);
	if (gScan->data)
		c_free(gScan->data);
	c_free(aps);
}

typedef struct xsWiFiRecord xsWiFiRecord;
typedef xsWiFiRecord *xsWiFi;

struct xsWiFiRecord {
	xsWiFi				next;
	xsSlot				obj;
	uint8_t				haveCallback;
};

static xsWiFi gWiFi;

static void wifiEventPending(modTimer timer, void *refcon, uint32_t refconSize)
{
	xsWiFi walker;
	system_event_id_t event_id = *(system_event_id_t *)refcon;
	const char *message;
	xsMachine *the = gThe;

	switch (event_id){
		case SYSTEM_EVENT_STA_START:			message = "start"; break;
		case SYSTEM_EVENT_STA_CONNECTED:		message = "connect"; break;
		case SYSTEM_EVENT_STA_DISCONNECTED:		message = "disconnect"; break;
		case SYSTEM_EVENT_STA_GOT_IP:			message = "gotIP"; break;
		case SYSTEM_EVENT_SCAN_DONE:			reportScan(); return;
		default: return;
	}

	for (walker = gWiFi; NULL != walker; walker = walker->next) {
		if (!walker->haveCallback)
			continue;

		xsBeginHost(the);
			xsCall1(walker->obj, xsID("callback"), xsString(message));
		xsEndHost(the);
	}
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
		xsCall1(xsThis, xsID("build"), xsArg(0));
	else if (2 == argc)
		xsCall2(xsThis, xsID("build"), xsArg(0), xsArg(1));
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
	}
	else if (wifi->haveCallback) {
		xsmcDelete(xsThis, xsID("callback"));
		wifi->haveCallback = false;
	}

	if (!xsmcTest(xsArg(0)))
		return;

	wifi->haveCallback = true;

	wifi->next = gWiFi;
	gWiFi = wifi;

	wifi->obj = xsThis;
	xsmcSet(xsThis, xsID("callback"), xsArg(0));

	xsCall1(wifi->obj, xsID("callback"), xsString("init"));		//@@ this should be unnecessary
}

static esp_err_t doWiFiEvent(void *ctx, system_event_t *event)
{
	wifi_config_t wifi_config;

    switch(event->event_id) {
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
			break;
		default:
			return ESP_OK;
	}

	modTimerAdd(0, 0, wifiEventPending, &event->event_id, sizeof(event->event_id));

	return ESP_OK;
}

void initWiFi(void)
{
	if (gWiFiState > 0) return;

	ESP_ERROR_CHECK( esp_event_loop_set_cb(doWiFiEvent, NULL) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_start() );

	gWiFiState = 1;
}

void xs_wifi_accessPoint(xsMachine *the)
{
}
