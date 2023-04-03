/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "pico/cyw43_arch.h"

#include "modTimer.h"

void wlanMakeCallback(uint32_t val);

enum {
	STATION_CONNECTED = 0,
	STATION_WRONG_PASSWORD,
	STATION_DISCONNECTED,
	STATION_GOT_IP
};
int8_t gWiFiStatus = STATION_DISCONNECTED;
int8_t gDisconnectReason = 0;	// -1 = password rejected
int8_t gWiFiState = -1;

struct wifiScanRecord {
	xsSlot		callback;
	xsMachine	*the;
	int			canceled;
};
typedef struct wifiScanRecord wifiScanRecord;

static wifiScanRecord *gScan = NULL;
static int scanInProgress = 0;

typedef struct xsWiFiRecord xsWiFiRecord;
typedef xsWiFiRecord *xsWiFi;

struct xsWiFiRecord {
	xsWiFi				next;
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				haveCallback;
	int8_t				disconnectReason;
};

static xsWiFi gWiFi = NULL;
char gSSID[33] = {0};

struct apScanRecord {
	struct apScanRecord *next;
	uint8_t		ssid[33];
	int8_t		rssi;
	uint8_t		channel;
	uint8_t		bssid[6];
	uint32_t	auth_mode;
};
typedef struct apScanRecord apScanRecord;
static apScanRecord *gScanResult = NULL;

enum {
	WIFI_OFF = 0,
	WIFI_STA = 1,
	WIFI_AP = 2
};
int gWiFiMode = WIFI_OFF;

static modTimer gMonitorWiFiTimer = NULL;
void wifiMonitorCallback(modTimer timer, void *refcon, int refconSize);

static void deleteScanResults() {
	apScanRecord *rec;
	while (NULL != (rec = gScanResult)) {
		gScanResult = rec->next;
		c_free(rec);
	}
}

void xs_wifi_set_mode(xsMachine *the)
{
	int mode = xsmcToInteger(xsArg(0));

	if (WIFI_OFF == mode)
		pico_unuse_cyw43();

	if (gWiFiMode == mode)
		return;

	int err = pico_use_cyw43();
	if (err) {
		modLog_transmit("pico_use_cyw43 failed:");
		modLogInt(err);
	}


	if (WIFI_STA == mode) {
		cyw43_arch_enable_sta_mode();
	}
	else if (WIFI_AP == mode) {
		// we need an ssid and password to set this up
	}
	else
		xsUnknownError("bad wifi mode");

	gWiFiMode = mode;
}

void xs_wifi_get_mode(xsMachine *the)
{
	xsmcSetInteger(xsResult, gWiFiMode);
}

static void scanResultRcvd(void *the, void *refcon, uint8_t *message, uint16_t msgLen)
{
	wifiScanRecord *scan = gScan;
	gScan = NULL;
	xsBeginHost(the);
	if (!scan || scan->canceled)
		goto done;

	apScanRecord *rec;
	while (NULL != (rec = gScanResult)) {
		xsmcVars(2);
		xsTry {
			xsmcSetNewObject(xsVar(1));

			xsmcSetString(xsVar(0), rec->ssid);
			xsmcSet(xsVar(1), xsID_ssid, xsVar(0));

			xsmcSetInteger(xsVar(0), rec->rssi);
			xsmcSet(xsVar(1), xsID_rssi, xsVar(0));

			xsmcSetInteger(xsVar(0), rec->channel);
			xsmcSet(xsVar(1), xsID_channel, xsVar(0));

			xsmcSetArrayBuffer(xsVar(0), rec->bssid, 6);
			xsmcSet(xsVar(1), xsID_bssid, xsVar(0));

			switch (rec->auth_mode) {
				case CYW43_AUTH_OPEN:
					xsmcSetString(xsVar(0), "none");
					break;
				case CYW43_AUTH_WPA_TKIP_PSK:
					xsmcSetString(xsVar(0), "wpa_tkip");
					break;
				case CYW43_AUTH_WPA2_AES_PSK:
					xsmcSetString(xsVar(0), "wpa2_psk");
					break;
				case CYW43_AUTH_WPA2_MIXED_PSK:
					xsmcSetString(xsVar(0), "wpa2_mixed");
					break;
				default:
					xsmcSetString(xsVar(0), "unknown");
					break;
			}
			xsmcSet(xsVar(1), xsID_authentication, xsVar(0));

			if (scan->canceled || gWiFiMode == 0) {
			}
			else {
				xsCallFunction1(scan->callback, xsGlobal, xsVar(1));
			}
		}
		xsCatch {
		}

		gScanResult = rec->next;
		c_free(rec);
	}

done:
	if (scan)
		xsCallFunction1(scan->callback, xsGlobal, xsNull);

	xsEndHost(the);

	if (scan) {
		xsForget(scan->callback);
	}
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
	apScanRecord *rec;

	if (result) {
		rec = (apScanRecord *)c_malloc(sizeof(apScanRecord));
		rec->next = gScanResult;
		gScanResult = rec;
		c_memcpy(&rec->ssid, result->ssid, 32);
		rec->rssi = result->rssi;
		rec->channel = result->channel;
		c_memcpy(&rec->bssid, result->bssid, 6);
		rec->auth_mode = result->auth_mode;
	}
	return 0;
}

void xs_wifi_scan(xsMachine *the)
{
	if (WIFI_STA != gWiFiMode)
		xsUnknownError("can't scan");

	if (gScan)
		xsUnknownError("unfinished wifi scan pending");

	if (xsmcArgc) {
		xsmcVars(1);

		if (xsmcHas(xsArg(0), xsID_hidden)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
//			config.show_hidden = xsmcTest(xsVar(0));
		}

		if (xsmcHas(xsArg(0), xsID_channel)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_channel);
//			config.channel = xsmcToInteger(xsVar(0));
		}
	}

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
	gScan->callback = xsArg(1);
	gScan->the = the;

	while (1) {
		if (!scanInProgress) {
			cyw43_wifi_scan_options_t scan_options = {0};
			int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
			if (err == 0)
				scanInProgress = true;
			else {
				c_free(gScan);
				gScan = NULL;
				xsUnknownError("scan request failed");
			}
		}
		else if (!cyw43_wifi_scan_active(&cyw43_state)) {
			scanInProgress = false;
			// scan complete
			if (!gScan) {
				deleteScanResults();
				return;
			}
			modMessagePostToMachine(gScan->the, NULL, 0, scanResultRcvd, NULL);
			break;
		}
#if CYW43_LWIP
		cyw43_arch_poll();
#endif
		modDelayMilliseconds(250);
	}

	xsRemember(gScan->callback);
}

void xs_wifi_connect(xsMachine *the)
{
	char *str;
	int argc = xsmcArgc;
	int channel = -1;

	if (gWiFiState > 1) {
		if (gWiFiStatus != STATION_DISCONNECTED) {
			gWiFiState = 2;
			gDisconnectReason = 0;
			// wifi_disconnect()
			gWiFiStatus = STATION_DISCONNECTED;
			wlanMakeCallback(gWiFiStatus);
		}
	}

	if (0 == argc)
		return;

	xsmcVars(2);

	uint8_t ssid[33] = {0};
	uint8_t password[65] = {0};

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	if (!xsmcTest(xsVar(0)))
		xsUnknownError("ssid required");
	str = xsmcToString(xsVar(0));
	if (c_strlen(str) > 32)
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(ssid, str, c_strlen(str));

	xsmcGet(xsVar(0), xsArg(0), xsID_password);
	if (xsmcTest(xsVar(0))) {
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > 64)
			xsUnknownError("password too long - 64 bytes max");
		c_memcpy(password, str, c_strlen(str));
	}

/*
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
*/

	int err = cyw43_arch_wifi_connect_async(ssid, password, CYW43_AUTH_WPA2_AES_PSK);
	if (0 == err) {
		c_strcpy(gSSID, ssid);
		if (!gMonitorWiFiTimer) {
			gMonitorWiFiTimer = modTimerAdd(250, 250, wifiMonitorCallback, NULL, 0);
		}
//		wlanMakeCallback(STATION_CONNECTED);
//		wlanMakeCallback(STATION_GOT_IP);
	}
	else {
		gSSID[0] = 0;
		modLog_transmit("wifi_connect failed - err:");
		modLogInt(err);
		if (err == CYW43_LINK_BADAUTH)
			wlanMakeCallback(STATION_WRONG_PASSWORD);
		else
			wlanMakeCallback(STATION_DISCONNECTED);
	}
}

static void wifiEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsWiFi wifi = refcon;
	uint32_t status = *(uint32_t *)message;

	switch (status) {
		case STATION_CONNECTED:
			gWiFiState = 4;
			message = "connect";
			break;
		case STATION_WRONG_PASSWORD:
			gWiFiState = 2;
			gDisconnectReason = -1;
			message = "disconnect";
			break;
		case STATION_DISCONNECTED:
			gWiFiState = 2;
			message = "disconnect";
			break;
		case STATION_GOT_IP:
			gWiFiState = 5;
			message = "gotIP";
			break;
	}

	xsBeginHost(the);
		if (gWiFiState == 2)
			xsCall1(wifi->obj, xsID_callback, xsString(message));
		else {
			xsmcSetInteger(xsResult, gDisconnectReason);
			xsCall2(wifi->obj, xsID_callback, xsString(message), xsResult);
		}
	xsEndHost(the);
}

void wlanMakeCallback(uint32_t val)
{
	xsWiFi walker;
	for (walker = gWiFi; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, (uint8_t *)&val, sizeof(val), wifiEventPending, walker);
}

int lastLinkStatus = CYW43_LINK_DOWN;
void wifiMonitorCallback(modTimer timer, void *refcon, int refconSize)
{
	int status;
	int do_disconnect = 0;

	status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
	if (status != lastLinkStatus) {
		switch (status) {
			case CYW43_LINK_DOWN:
				do_disconnect = 1;
				break;
			case CYW43_LINK_JOIN:
				if (gWiFiState == 5)
					do_disconnect = 1;
				else
					wlanMakeCallback(STATION_CONNECTED);
				break;
			case CYW43_LINK_NOIP:
				break;
			case CYW43_LINK_UP:
				if (gWiFiState != 5)
					wlanMakeCallback(STATION_GOT_IP);
				break;
			case CYW43_LINK_FAIL:
				do_disconnect = 1;
				break;
			case CYW43_LINK_NONET:
				do_disconnect = 1;
				break;
			case CYW43_LINK_BADAUTH:
				do_disconnect = 1;
				break;
		}
		lastLinkStatus = status;
	}

	if (do_disconnect) {
		gWiFiStatus = STATION_DISCONNECTED;
		wlanMakeCallback(gWiFiStatus);
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

//	if (NULL == gWiFi)
//		wifi_set_event_handler_cb(doWiFiEvent);

	wifi->next = gWiFi;
	gWiFi = wifi;

	wifi->obj = xsThis;
	xsmcSet(xsThis, xsID_callback, xsArg(0));
}

void xs_wifi_accessPoint(xsMachine *the)
{
modLog_transmit("xs_wifi_accessPoint - not implemented\n");
/* not yet
    struct softap_config config;
	char *str;
	int ret;
	uint8_t station = 0;

	c_memset(&config, 0, sizeof(config));

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	str = xsmcToString(xsVar(0));
	config.ssid_len = c_strlen(str);
	if (config.ssid_len > (sizeof(config.ssid) - 1))
		xsUnknownError("ssid too long - 32 bytes max");
	c_memcpy(config.ssid, str, config.ssid_len);

	config.authmode = AUTH_OPEN;
	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > (sizeof(config.password) - 1))
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
*/
}

