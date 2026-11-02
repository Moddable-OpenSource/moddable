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

#include "user_interface.h"
#include "wifi-esp-events.h"

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
} xsAPRecord;

static xsAP gAPInstance;

#define AP_EVENT_READY 0xFFFF

typedef struct {
	uint32_t	event;
	uint8_t		mac[6];
} APEventMsg;

static void doAPEvent(System_Event_t *msg);
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

static AUTH_MODE stringToAuthMode(const char *str)
{
	if (0 == c_strcmp(str, "none")) return AUTH_OPEN;
	if (0 == c_strcmp(str, "wpa2_psk")) return AUTH_WPA2_PSK;
	if (0 == c_strcmp(str, "wpa_wpa2_psk")) return AUTH_WPA_WPA2_PSK;
	return AUTH_MAX;
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

static const xsHostHooks ICACHE_RODATA_ATTR xsStationHooks = {
	xs_apstation_destructor,
	xs_apstation_mark,
	C_NULL
};

void xs_apstation_close(xsMachine *the)
{
	xsStation station = xsmcGetHostData(xsThis);
	if (!station) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsStationHooks);

	wifi_softap_deauth(station->mac);
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

	struct station_info *info = wifi_softap_get_station_info();
	for (struct station_info *walker = info; walker; walker = walker->next.stqe_next) {
		if (0 == c_memcmp(walker->bssid, station->mac, sizeof(station->mac))) {
			if (walker->ip.addr) {
				xsResult = xsStringBuffer(NULL, 15);
				ipaddr_ntoa_r(&walker->ip, xsmcToString(xsResult), 16);
			}
			break;
		}
	}
	wifi_softap_free_station_info();
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

static const xsHostHooks ICACHE_RODATA_ATTR xsAPHooks = {
	xs_wifiaccesspoint_destructor,
	xs_wifiaccesspoint_mark,
	C_NULL
};

void xs_wifiaccesspoint(xsMachine *the)
{
	if (gAPInstance)
		xsUnknownError("access point already open");

	struct softap_config config = {0};
	xsmcVars(1);

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_SSID))
		xsUnknownError("SSID required");
	char ssid[sizeof(config.ssid) + 1];
	xsmcToStringBuffer(xsVar(0), ssid, sizeof(ssid));
	size_t ssidLen = c_strlen(ssid);
	if (0 == ssidLen)
		xsRangeError("SSID empty");
	c_memcpy(config.ssid, ssid, ssidLen);
	config.ssid_len = ssidLen;

	uint8_t hasPassword = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_password)) {
		char password[sizeof(config.password) + 1];
		xsmcToStringBuffer(xsVar(0), password, sizeof(password));
		size_t plen = c_strlen(password);
		if (plen) {
			if (plen < 8)
				xsRangeError("password too short - 8 bytes min");
			c_memcpy(config.password, password, plen);
			hasPassword = 1;
		}
	}

	config.authmode = hasPassword ? AUTH_WPA2_PSK : AUTH_OPEN;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_authentication)) {
		char auth[24];
		xsmcToStringBuffer(xsVar(0), auth, sizeof(auth));
		AUTH_MODE mode = stringToAuthMode(auth);
		if (AUTH_MAX == mode)
			xsRangeError("invalid authentication");
		if ((AUTH_OPEN != mode) && !hasPassword)
			xsUnknownError("password required for authentication mode");
		config.authmode = mode;
	}

	config.channel = 1;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_channel)) {
		int channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsRangeError("invalid channel");
		config.channel = channel;
	}

	config.ssid_hidden = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_hidden))
		config.ssid_hidden = xsmcTest(xsVar(0)) ? 1 : 0;

	config.max_connection = 4;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_max)) {
		int max = xsmcToInteger(xsVar(0));
		if ((max < 1) || (max > 4))
			xsRangeError("invalid max");
		config.max_connection = max;
	}

	config.beacon_interval = 100;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_interval)) {
		int interval = xsmcToInteger(xsVar(0));
		if ((interval < 100) || (interval > 60000))
			xsRangeError("invalid interval");
		config.beacon_interval = interval;
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

	// Enable AP mode (OR-in SOFTAP so station can coexist)
	ETS_UART_INTR_DISABLE();
	uint8_t setMode = wifi_set_opmode_current(wifi_get_opmode() | SOFTAP_MODE);
	ETS_UART_INTR_ENABLE();
	if (!setMode)
		xsUnknownError("wifi_set_opmode_current failed");

	wifi_set_sleep_type(NONE_SLEEP_T);

	ETS_UART_INTR_DISABLE();
	uint8_t setConfig = wifi_softap_set_config_current(&config);
	ETS_UART_INTR_ENABLE();
	if (!setConfig)
		xsUnknownError("wifi_softap_set_config_current failed");

	if (DHCP_STARTED != wifi_softap_dhcps_status()) {
		if (!wifi_softap_dhcps_start())
			xsUnknownError("wifi_softap_dhcps_start failed");
	}

	struct ip_info info;
	if (!wifi_get_ip_info(SOFTAP_IF, &info) || !info.ip.addr)
		xsUnknownError("AP IP configuration failed");

	wifi_esp_events_addListener(doAPEvent);

	gAPInstance = ap;
	ap->connection = 400;

	if (ap->onChanged) {
		APEventMsg m = {0};
		m.event = AP_EVENT_READY;
		ap->useCount += 1;
		modMessagePostToMachine(the, (uint8_t *)&m, sizeof(m), apEventDeliver, ap);
	}
}

static void doAPEvent(System_Event_t *msg)
{
	xsAP ap = gAPInstance;
	if (!ap) return;

	APEventMsg m = {0};
	m.event = msg->event;

	if (EVENT_SOFTAPMODE_STACONNECTED == msg->event) {
		if (!ap->onConnect) return;
		c_memcpy(m.mac, msg->event_info.sta_connected.mac, sizeof(m.mac));
	}
	else if (EVENT_SOFTAPMODE_STADISCONNECTED == msg->event) {
		c_memcpy(m.mac, msg->event_info.sta_disconnected.mac, sizeof(m.mac));
	}
	else
		return;

	ap->useCount += 1;
	modMessagePostToMachine(ap->the, (uint8_t *)&m, sizeof(m), apEventDeliver, ap);
}

static void apEventDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsAP ap = refcon;
	APEventMsg *msg = (APEventMsg *)msgIn;

	if (ap->closed)
		goto bail;

	if (AP_EVENT_READY == msg->event) {
		if (!ap->onChanged) goto bail;
		xsBeginHost(the);
			xsmcSetStringX(xsResult, (char *)"connection");
			xsCallFunction1(xsReference(ap->onChanged), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (EVENT_SOFTAPMODE_STACONNECTED == msg->event) {
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

			xsCallFunction1(xsReference(ap->onConnect), ap->obj, xsResult);
		xsEndHost(the);
	}
	else if (EVENT_SOFTAPMODE_STADISCONNECTED == msg->event) {
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
		// deauth all associated stations so clients notice we're going away
		for (xsStation walker = ap->stations; walker; walker = walker->next)
			wifi_softap_deauth(walker->mac);

		wifi_softap_dhcps_stop();

		uint8_t mode = wifi_get_opmode() & ~SOFTAP_MODE;
		wifi_set_opmode_current(mode);

		wifi_esp_events_removeListener(doAPEvent);
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

	struct ip_info info;
	if (!wifi_get_ip_info(SOFTAP_IF, &info) || !info.ip.addr)
		return;

	xsResult = xsStringBuffer(NULL, 15);
	ipaddr_ntoa_r(&info.ip, xsmcToString(xsResult), 16);
}

void xs_wifiaccesspoint_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	uint8_t mac[6];

	if (!wifi_get_macaddr(SOFTAP_IF, mac))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}

void xs_wifiaccesspoint_SSID_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	struct softap_config config;
	if (!wifi_softap_get_config(&config))
		return;

	char ssid[sizeof(config.ssid) + 1];
	if (config.ssid_len) {
		c_memcpy(ssid, config.ssid, config.ssid_len);
		ssid[config.ssid_len] = 0;
	}
	else {
		c_memcpy(ssid, config.ssid, sizeof(config.ssid));
		ssid[sizeof(config.ssid)] = 0;
	}
	xsmcSetString(xsResult, ssid);
}

void xs_wifiaccesspoint_channel_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAPHooks);
	struct softap_config config;
	if (!wifi_softap_get_config(&config))
		return;

	xsmcSetInteger(xsResult, config.channel);
}
