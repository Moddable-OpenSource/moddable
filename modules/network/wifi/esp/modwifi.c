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
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

#include "user_interface.h"

static void wifiScanComplete(void *arg, STATUS status);
static void deliverScanResults(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

struct aWiFiResultRecord {
	uint8 length;
	uint8 bssid[6];
	uint8 channel;
	sint8 rssi;
	uint8 authmode;		// instead of AUTH_MODE, to save space
	uint8 is_hidden;
	uint8 ssid[1];		// not null terminated... use record length to determine ssid length
};
typedef struct aWiFiResultRecord aWiFiResultRecord;
typedef struct aWiFiResultRecord *aWiFiResult;

struct wifiScanRecord {
	xsSlot			callback;
	xsMachine		*the;
	uint8_t			count;
	aWiFiResult		scan;
};
typedef struct wifiScanRecord wifiScanRecord;

static wifiScanRecord *gScan;

void xs_wifi_set_mode(xsMachine *the)
{
	int mode = xsmcToInteger(xsArg(0));
	wifi_set_opmode_current(mode);
}

void xs_wifi_get_mode(xsMachine *the)
{
	xsmcSetInteger(xsResult, wifi_get_opmode());
}

void xs_wifi_scan(xsMachine *the)
{
	struct scan_config config;
	uint8 mode = wifi_get_opmode();

	if ((STATION_MODE != mode) && (STATIONAP_MODE != mode))
		xsUnknownError("can't scan");

	if (gScan)
		xsUnknownError("unfinished wifi scan pending");

	c_memset(&config, 0, sizeof(config));
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

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
	gScan->callback = xsArg(1);
	gScan->the = the;

	if (!wifi_station_scan(&config, wifiScanComplete)) {
		c_free(gScan);
		gScan = NULL;
		xsUnknownError("scan request failed");
	}

	xsRemember(gScan->callback);
}

void xs_wifi_connect(xsMachine *the)
{
	struct station_config config;
	char *str;
	int argc = xsmcArgc;
	int channel = -1;

	if (STATION_IDLE != wifi_station_get_connect_status())
		wifi_station_disconnect();

	if (0 == argc)
		return;

	c_memset(&config, 0, sizeof(config));

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	if (!xsmcTest(xsVar(0)))
		xsUnknownError("ssid required");
	str = xsmcToString(xsVar(0));
	if (c_strlen(str) > sizeof(config.ssid))
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(config.ssid, str, c_strlen(str));

	xsmcGet(xsVar(0), xsArg(0), xsID_password);
	if (xsmcTest(xsVar(0))) {
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > sizeof(config.password))
			xsUnknownError("password too long - 64 bytes max");
		c_memcpy(config.password, str, c_strlen(str));
	}

	if (xsmcHas(xsArg(0), xsID_bssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bssid);
		if (sizeof(config.bssid) != xsmcGetArrayBufferLength(xsVar(0)))
			xsUnknownError("bssid must be 6 bytes");
		xsmcGetArrayBufferData(xsVar(0), 0, config.bssid, sizeof(config.bssid));
		config.bssid_set = 1;
	}

	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		channel = xsmcToInteger(xsVar(0));
	}

	wifi_set_opmode_current(STATION_MODE);

	wifi_station_set_config_current(&config);

	wifi_set_sleep_type(NONE_SLEEP_T);

	if (channel >= 0)
		wifi_set_channel(channel);

	wifi_station_connect();		// may not always be necessary
}

void wifiScanComplete(void *arg, STATUS status)
{
	if (OK == status) {
		struct bss_info *bss;
		uint16_t size = 0;

		// copy scan record for use in callback
		for (bss = arg; NULL != bss; bss = bss->next.stqe_next) {
			size += (sizeof(aWiFiResultRecord) - 1) + bss->ssid_len;
			gScan->count += 1;
		}

		if (size) {
			gScan->scan = c_malloc(size);
			if (gScan->scan) {
				aWiFiResult awr = (aWiFiResult)gScan->scan;
				for (bss = arg; NULL != bss; bss = bss->next.stqe_next) {
					awr->length = (sizeof(aWiFiResultRecord) - 1) + bss->ssid_len;
					c_memcpy(awr->bssid, bss->bssid, sizeof(bss->bssid));
					awr->channel = bss->channel;
					awr->rssi = bss->rssi;
					awr->authmode = bss->authmode;
					awr->is_hidden = bss->is_hidden;
					c_memcpy(awr->ssid, bss->ssid, bss->ssid_len);
					awr = (aWiFiResult)(awr->length + (uint8_t *)awr);
				}
			}
		}
	}

	modMessagePostToMachine(gScan->the, NULL, 0, deliverScanResults, NULL);
}

void deliverScanResults(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(2);
	if (gScan->scan) {
		aWiFiResult awr = gScan->scan;

		xsTry {
			while (gScan->count--) {
				xsmcSetNewObject(xsVar(1));

				xsmcSetStringBuffer(xsVar(0), awr->ssid, awr->length - (sizeof(aWiFiResultRecord) - 1));
				xsmcSet(xsVar(1), xsID_ssid, xsVar(0));

				xsmcSetInteger(xsVar(0), awr->rssi);
				xsmcSet(xsVar(1), xsID_rssi, xsVar(0));

				xsmcSetInteger(xsVar(0), awr->channel);
				xsmcSet(xsVar(1), xsID_channel, xsVar(0));

				xsmcSetBoolean(xsVar(0), awr->is_hidden);
				xsmcSet(xsVar(1), xsID_hidden, xsVar(0));

				xsmcSetArrayBuffer(xsVar(0), awr->bssid, sizeof(awr->bssid));
				xsmcSet(xsVar(1), xsID_bssid, xsVar(0));

				if (awr->authmode < AUTH_MAX) {
					if (AUTH_OPEN == awr->authmode)
						xsmcSetString(xsVar(0), "none");
					else if (AUTH_WEP == awr->authmode)
						xsmcSetString(xsVar(0), "wep");
					else if (AUTH_WPA_PSK == awr->authmode)
						xsmcSetString(xsVar(0), "wpa_psk");
					else if (AUTH_WPA2_PSK == awr->authmode)
						xsmcSetString(xsVar(0), "wpa2_psk");
					else // if (AUTH_WPA_WPA2_PSK == awr->authmode)
						xsmcSetString(xsVar(0), "wpa_wpa2_psk");

					xsmcSet(xsVar(1), xsID_authentication, xsVar(0));
				}

				xsCallFunction1(gScan->callback, xsGlobal, xsVar(1));

				awr = (aWiFiResult)(awr->length + (uint8_t *)awr);
			}
		}
		xsCatch {
		}

		c_free(gScan->scan);
	}

	xsResult = gScan->callback;
	xsForget(gScan->callback);
	c_free(gScan);
	gScan = NULL;

	xsCallFunction1(xsResult, xsGlobal, xsNull);		// end of scan

	xsEndHost(the);
}

typedef struct xsWiFiRecord xsWiFiRecord;
typedef xsWiFiRecord *xsWiFi;

struct xsWiFiRecord {
	xsWiFi				next;
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				haveCallback;
	int8_t				disconnectReason;
};

static xsWiFi gWiFi;

static void wifiEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsWiFi wifi = refcon;
	uint32 event = *(uint32 *)message;

	switch (event){
		case EVENT_STAMODE_CONNECTED:		message = "connect"; break;
		case EVENT_STAMODE_DISCONNECTED:	message = "disconnect"; break;
		case EVENT_STAMODE_GOT_IP:			message = "gotIP"; break;
		case EVENT_STAMODE_DHCP_TIMEOUT:	message = "lostIP"; break;
		default: return;
	}

	xsBeginHost(the);
		if (EVENT_STAMODE_DISCONNECTED != event)
			xsCall1(wifi->obj, xsID_callback, xsString(message));
		else {
			xsmcSetInteger(xsResult, wifi->disconnectReason);
			xsCall2(wifi->obj, xsID_callback, xsString(message), xsResult);
		}
	xsEndHost(the);
}

static void doWiFiEvent(System_Event_t *msg)
{
	xsWiFi walker;
	int8_t reason = 1;

	if (EVENT_STAMODE_DISCONNECTED == msg->event) {
		reason = msg->event_info.disconnected.reason;
		reason =	(REASON_AUTH_EXPIRE == reason) ||
					(REASON_MIC_FAILURE == reason) ||
					(REASON_4WAY_HANDSHAKE_TIMEOUT == reason) ||
					(REASON_GROUP_KEY_UPDATE_TIMEOUT == reason) ||
					(REASON_IE_IN_4WAY_DIFFERS == reason) ||
					(REASON_HANDSHAKE_TIMEOUT == reason) ||
					(REASON_AUTH_FAIL == reason);
		if (reason)
			reason = -1;
	}

	for (walker = gWiFi; NULL != walker; walker = walker->next) {
		if (reason <= 0)
			walker->disconnectReason = reason;

		if (walker->haveCallback)
			modMessagePostToMachine(walker->the, (uint8_t *)&msg->event, sizeof(msg->event), wifiEventPending, walker);
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

		if (NULL == gWiFi)
			wifi_set_event_handler_cb(NULL);
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

	if (NULL == gWiFi)
		wifi_set_event_handler_cb(doWiFiEvent);

	wifi->next = gWiFi;
	gWiFi = wifi;

	wifi->obj = xsThis;
	xsmcSet(xsThis, xsID_callback, xsArg(0));
}

void xs_wifi_accessPoint(xsMachine *the)
{
    struct softap_config config;
	char *str;
	int ret;
	uint8 station = 0;

	c_memset(&config, 0, sizeof(config));

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	str = xsmcToString(xsVar(0));
	config.ssid_len = c_strlen(str);
	if (config.ssid_len > sizeof(config.ssid))
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(config.ssid, str, config.ssid_len);

	config.authmode = AUTH_OPEN;
	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > sizeof(config.password))
			xsUnknownError("password too long - 64 bytes max");
		if (c_strlen(str) < 8)
			xsUnknownError("password too short - 8 bytes min");
		if (!c_isEmpty(str)) {
			c_memcpy(config.password, str, c_strlen(str));
			config.authmode = AUTH_WPA2_PSK;
		}
	}

	config.channel = 1;
	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		config.channel = xsmcToInteger(xsVar(0));
		if ((config.channel < 1) || (config.channel > 13))
			xsUnknownError("invalid channel");
	}

    config.ssid_hidden = 0;
	if (xsmcHas(xsArg(0), xsID_hidden)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
		config.ssid_hidden = xsmcTest(xsVar(0));
	}

    config.max_connection = 4;
	if (xsmcHas(xsArg(0), xsID_max)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_max);
		config.max_connection = xsmcToInteger(xsVar(0));
	}

    config.beacon_interval = 100;
	if (xsmcHas(xsArg(0), xsID_interval)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_interval);
		config.beacon_interval = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_station)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_station);
		station = xsmcToBoolean(xsVar(0));
	}

	ETS_UART_INTR_DISABLE();
	ret = wifi_set_opmode_current(station ? STATIONAP_MODE : SOFTAP_MODE);
	ETS_UART_INTR_ENABLE();
	if (!ret)
		xsUnknownError("wifi_set_opmode_current failed");

	wifi_set_sleep_type(NONE_SLEEP_T);

	ETS_UART_INTR_DISABLE();
	ret = wifi_softap_set_config_current(&config);
	ETS_UART_INTR_ENABLE();
	if (!ret)
		xsUnknownError("wifi_softap_set_config_current failed");

    if (DHCP_STARTED != wifi_softap_dhcps_status()) {
        if (!wifi_softap_dhcps_start())
			xsUnknownError("wifi_softap_dhcps_status failed");
    }

    // check IP config
    struct ip_info ip;
    if (!wifi_get_ip_info(SOFTAP_IF, &ip))
        xsUnknownError("wifi_get_ip_info failed!\n");

	if (0 == ip.ip.addr)
        xsUnknownError("IP config bad when starting Wi-Fi!\n");
}
