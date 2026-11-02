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

#include "esp_wifi.h"
#include "esp_netif.h"

typedef struct xsStationRecord *xsStation;
typedef struct xsStationRecord {
	xsStation		next;
	xsSlot			*obj;
	uint8_t			mac[6];
	esp_ip4_addr_t	ip;
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
} xsAPRecord;

static esp_netif_t *gAP;
static xsAP gAPInstance;
static uint8_t gAPEventHandlerRegistered;

typedef struct {
	int32_t		event_id;
	uint8_t		mac[6];
	esp_ip4_addr_t	ip;
} APEventMsg;

static void doAPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void initAP(xsMachine *the);

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

static wifi_auth_mode_t stringToAuthMode(const char *str)
{
	if (0 == c_strcmp(str, "none")) return WIFI_AUTH_OPEN;
	if (0 == c_strcmp(str, "wpa2_psk")) return WIFI_AUTH_WPA2_PSK;
	if (0 == c_strcmp(str, "wpa_wpa2_psk")) return WIFI_AUTH_WPA_WPA2_PSK;
	if (0 == c_strcmp(str, "wpa3_psk")) return WIFI_AUTH_WPA3_PSK;
	if (0 == c_strcmp(str, "wpa2_wpa3_psk")) return WIFI_AUTH_WPA2_WPA3_PSK;
	return WIFI_AUTH_MAX;
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

	uint16_t aid;
	if (ESP_OK == esp_wifi_ap_get_sta_aid(station->mac, &aid))
		esp_wifi_deauth_sta(aid);
	// removal from AP list and onDisconnect happen from the STADISCONNECTED event
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

	if (0 == station->ip.addr) return;

	xsResult = xsStringBuffer(NULL, 16);
	esp_ip4addr_ntoa(&station->ip, xsmcToString(xsResult), 17);
}

// -------- WiFiAccessPoint --------

void xs_wifiaccesspoint_destructor(void *data)
{
	xsAP ap = (xsAP)data;
	if (!ap) return;

	if (__atomic_sub_fetch(&ap->useCount, 1, __ATOMIC_SEQ_CST) > 0)
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

	wifi_config_t config = {0};
	xsmcVars(1);

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");
	char ssid[sizeof(config.ap.ssid) + 1];
	xsmcToStringBuffer(xsVar(0), ssid, sizeof(ssid));
	size_t ssidLen = c_strlen(ssid);
	if (0 == ssidLen)
		xsRangeError("SSID empty");
	c_memcpy(config.ap.ssid, ssid, ssidLen);
	config.ap.ssid_len = ssidLen;

	uint8_t hasPassword = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_password)) {
		char password[sizeof(config.ap.password) + 1];
		xsmcToStringBuffer(xsVar(0), password, sizeof(password));
		size_t plen = c_strlen(password);
		if (plen) {
			if (plen < 8)
				xsRangeError("password too short - 8 bytes min");
			c_memcpy(config.ap.password, password, plen);
			hasPassword = 1;
		}
	}

	config.ap.authmode = hasPassword ? WIFI_AUTH_WPA2_WPA3_PSK : WIFI_AUTH_OPEN;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_authentication)) {
		char auth[24];
		xsmcToStringBuffer(xsVar(0), auth, sizeof(auth));
		wifi_auth_mode_t mode = stringToAuthMode(auth);
		if (WIFI_AUTH_MAX == mode)
			xsRangeError("invalid authentication");
		if ((WIFI_AUTH_OPEN != mode) && !hasPassword)
			xsUnknownError("password required for authentication mode");
		config.ap.authmode = mode;
	}

	config.ap.channel = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_channel)) {
		int channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsRangeError("invalid channel");
		config.ap.channel = channel;
	}

	config.ap.ssid_hidden = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_hidden))
		config.ap.ssid_hidden = xsmcTest(xsVar(0)) ? 1 : 0;

	config.ap.max_connection = 4;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_max)) {
		int max = xsmcToInteger(xsVar(0));
		if ((max < 1) || (max > 10))
			xsRangeError("invalid max");
		config.ap.max_connection = max;
	}

	config.ap.beacon_interval = 100;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_interval)) {
		int interval = xsmcToInteger(xsVar(0));
		if ((interval < 100) || (interval > 60000))
			xsRangeError("invalid interval");
		config.ap.beacon_interval = interval;
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
	xsSetHostHooks(xsThis, &xsAPHooks);
	xsRemember(ap->obj);

	initAP(the);

	if (ESP_OK != esp_wifi_set_config(WIFI_IF_AP, &config))
		xsUnknownError("set AP config failed");

	gAPInstance = ap;		// set before start so AP_START event is captured

	esp_err_t startErr = esp_wifi_start();
	if ((ESP_OK != startErr) && (ESP_ERR_WIFI_NOT_STOPPED != startErr)) {
		gAPInstance = C_NULL;
		xsUnknownError("wifi start failed");
	}
}

static void initAP(xsMachine *the)
{
	wifi_mode_t currentMode = WIFI_MODE_NULL;
	esp_err_t err = esp_wifi_get_mode(&currentMode);

	if (ESP_OK == err) {
		gAP = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
		if (!gAP)
			gAP = esp_netif_create_default_wifi_ap();

		wifi_mode_t newMode = ((WIFI_MODE_STA == currentMode) || (WIFI_MODE_APSTA == currentMode))
			? WIFI_MODE_APSTA
			: WIFI_MODE_AP;

		if (newMode != currentMode) {
			if (ESP_OK != esp_wifi_set_mode(newMode))
				xsUnknownError("set mode failed");
		}
	}
	else {
		esp_netif_init();
		esp_err_t loopErr = esp_event_loop_create_default();
		if ((ESP_OK != loopErr) && (ESP_ERR_INVALID_STATE != loopErr))
			xsUnknownError("event loop create failed");

		gAP = esp_netif_create_default_wifi_ap();
		esp_netif_create_default_wifi_sta();		// so future station use finds its netif

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		cfg.nvs_enable = 0;
		if (ESP_OK != esp_wifi_init(&cfg))
			xsUnknownError("wifi init failed");
		esp_wifi_set_storage(WIFI_STORAGE_RAM);
		if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_AP))
			xsUnknownError("set mode failed");
	}

	if (!gAPEventHandlerRegistered) {
		static const int32_t events[] = { WIFI_EVENT_AP_START, WIFI_EVENT_AP_STOP, WIFI_EVENT_AP_STACONNECTED, WIFI_EVENT_AP_STADISCONNECTED };
		for (int i = 0; i < 4; i++)
			esp_event_handler_register(WIFI_EVENT, events[i], doAPEvent, C_NULL);
		esp_event_handler_register(IP_EVENT, IP_EVENT_ASSIGNED_IP_TO_CLIENT, doAPEvent, C_NULL);
		gAPEventHandlerRegistered = 1;
	}
}

static void doAPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	xsAP ap = gAPInstance;
	if (!ap) return;

	APEventMsg msg = {0};
	msg.event_id = event_id;

	if (IP_EVENT == event_base) {
		if (IP_EVENT_ASSIGNED_IP_TO_CLIENT != event_id) return;
		ip_event_assigned_ip_to_client_t *evt = (ip_event_assigned_ip_to_client_t *)event_data;
		c_memcpy(msg.mac, evt->mac, sizeof(msg.mac));
		msg.ip = evt->ip;
	}
	else if (WIFI_EVENT_AP_START == event_id) {
		if (400 == ap->connection) return;
		ap->connection = 400;
		if (!ap->onChanged) return;
	}
	else if (WIFI_EVENT_AP_STOP == event_id) {
		if (200 == ap->connection) return;
		ap->connection = 200;
		if (!ap->onChanged) return;
	}
	else if (WIFI_EVENT_AP_STACONNECTED == event_id) {
		if (!ap->onConnect) return;
		wifi_event_ap_staconnected_t *evt = (wifi_event_ap_staconnected_t *)event_data;
		c_memcpy(msg.mac, evt->mac, sizeof(msg.mac));
	}
	else if (WIFI_EVENT_AP_STADISCONNECTED == event_id) {
		// always deliver so we can clean up the tracking list, even if there's no user callback
		wifi_event_ap_stadisconnected_t *evt = (wifi_event_ap_stadisconnected_t *)event_data;
		c_memcpy(msg.mac, evt->mac, sizeof(msg.mac));
	}
	else
		return;

	__atomic_add_fetch(&ap->useCount, 1, __ATOMIC_SEQ_CST);
	modMessagePostToMachine(ap->the, (uint8_t *)&msg, sizeof(msg), apEventDeliver, ap);
}

static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsAP ap = refcon;
	APEventMsg *msg = (APEventMsg *)msgIn;

	if (ap->closed)
		goto bail;

	if ((WIFI_EVENT_AP_START == msg->event_id) || (WIFI_EVENT_AP_STOP == msg->event_id)) {
		xsBeginHost(the);
			xsmcSetStringX(xsResult, (char *)"connection");
			xsCallFunction1(xsReference(ap->onChanged), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (WIFI_EVENT_AP_STACONNECTED == msg->event_id) {
		xsBeginHost(the);
			xsStation station = c_calloc(1, sizeof(xsStationRecord));
			if (!station)
				xsUnknownError("no memory");
			c_memcpy(station->mac, msg->mac, sizeof(station->mac));

			xsResult = xsNewHostInstance(xsReference(ap->stationPrototype));
			station->obj = xsmcToReference(xsResult);
			xsmcSetHostData(xsResult, station);
			xsSetHostHooks(xsResult, &xsStationHooks);

			station->next = ap->stations;
			ap->stations = station;

			xsCallFunction1(xsReference(ap->onConnect), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (WIFI_EVENT_AP_STADISCONNECTED == msg->event_id) {
		xsStation station = findStationByMAC(ap, msg->mac);
		if (!station)
			goto bail;

		// unlink from list
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

			// detach the record from the JS instance and free it — matches BLE server pattern
			xsResult = xsReference(station->obj);
			xsmcSetHostData(xsResult, C_NULL);
			xsSetHostDestructor(xsResult, C_NULL);
			c_free(station);
		xsEndHost(the);
	}
	else if (IP_EVENT_ASSIGNED_IP_TO_CLIENT == msg->event_id) {
		xsStation station = findStationByMAC(ap, msg->mac);
		if (station)
			station->ip = msg->ip;
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
		wifi_mode_t mode;
		if (ESP_OK == esp_wifi_get_mode(&mode)) {
			if ((WIFI_MODE_AP == mode) || (WIFI_MODE_APSTA == mode))
				esp_wifi_deauth_sta(0);		// deauth all associated stations
			if (WIFI_MODE_APSTA == mode)
				esp_wifi_set_mode(WIFI_MODE_STA);
			else if (WIFI_MODE_AP == mode)
				esp_wifi_set_mode(WIFI_MODE_NULL);
		}
		gAPInstance = C_NULL;
	}

	// detach any tracked stations
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
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	xsmcVars(1);
	if (xsmcGet(xsVar(0), xsArg(0), xsID_portal)) {
		char *url = xsmcToString(xsVar(0));
		esp_netif_dhcps_stop(gAP);
		esp_err_t err = esp_netif_dhcps_option(gAP, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, url, c_strlen(url));
		if (err != ESP_OK)
			xsUnknownError("can't set portal %d", (int)err);
		err = esp_netif_dhcps_start(gAP);
		if (err != ESP_OK)
			xsUnknownError("can't restart dhcp %d", (int)err);
	}
}

void xs_wifiaccesspoint_connection_get(xsMachine *the)
{
	xsAP ap = xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	xsmcSetInteger(xsResult, ap->connection);
}

void xs_wifiaccesspoint_address_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);

	if (!gAP) return;

	esp_netif_ip_info_t info;
	if ((ESP_OK == esp_netif_get_ip_info(gAP, &info)) && info.ip.addr) {
		xsResult = xsStringBuffer(NULL, 39);
		esp_ip4addr_ntoa(&info.ip, xsmcToString(xsResult), 40);
	}
}

void xs_wifiaccesspoint_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	uint8_t mac[6];

	if (ESP_OK != esp_wifi_get_mac(WIFI_IF_AP, mac))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifiaccesspoint_SSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	wifi_config_t config;
	if (ESP_OK != esp_wifi_get_config(WIFI_IF_AP, &config))
		return;

	char ssid[sizeof(config.ap.ssid) + 1];
	if (config.ap.ssid_len) {
		c_memcpy(ssid, config.ap.ssid, config.ap.ssid_len);
		ssid[config.ap.ssid_len] = 0;
	}
	else {
		c_memcpy(ssid, config.ap.ssid, sizeof(config.ap.ssid));
		ssid[sizeof(config.ap.ssid)] = 0;
	}
	xsmcSetString(xsResult, ssid);
}

void xs_wifiaccesspoint_channel_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	wifi_config_t config;
	if (ESP_OK != esp_wifi_get_config(WIFI_IF_AP, &config))
		return;

	xsmcSetInteger(xsResult, config.ap.channel);
}
