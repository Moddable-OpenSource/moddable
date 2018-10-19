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
#include "xsesp.h"

#include "string.h"

#if ESP32
	#include "esp_wifi.h"
#else
	#include "user_interface.h"
#endif

void twoHex(uint8_t value, char *out)
{
	static const char *gHex = "0123456789ABCDEF";
	*out++ = espRead8(gHex + (value >> 4));
	*out++ = espRead8(gHex + (value & 15));
}

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

	if (0 == espStrCmp(prop, "IP")) {
#if ESP32
		wifi_mode_t mode;
		tcpip_adapter_ip_info_t info = {0};

		esp_wifi_get_mode(&mode);
		if ((ESP_OK == tcpip_adapter_get_ip_info(mode == WIFI_MODE_AP ? TCPIP_ADAPTER_IF_AP : TCPIP_ADAPTER_IF_STA, &info)) && info.ip.addr) {
#else
		struct ip_info info;

		if (wifi_get_ip_info((SOFTAP_MODE == wifi_get_opmode()) ? SOFTAP_IF : STATION_IF, &info)) {
#endif
			char *out;
			xsResult = xsStringBuffer(NULL, 4 * 5);
			out = xsToString(xsResult);
			itoa(ip4_addr1(&info.ip), out, 10); out += strlen(out); *out++ = '.';
			itoa(ip4_addr2(&info.ip), out, 10); out += strlen(out); *out++ = '.';
			itoa(ip4_addr3(&info.ip), out, 10); out += strlen(out); *out++ = '.';
			itoa(ip4_addr4(&info.ip), out, 10); out += strlen(out); *out = 0;
		}

	}
	else if (0 == espStrCmp(prop, "MAC")) {
		uint8_t macaddr[6];
#if ESP32
		if (ESP_OK == esp_wifi_get_mac(ESP_IF_WIFI_STA, macaddr))
#else
		if (wifi_get_macaddr((SOFTAP_MODE == wifi_get_opmode()) ? SOFTAP_IF : STATION_IF, macaddr))
#endif
		{
			char *out;
			xsResult = xsStringBuffer(NULL, 18);
			out = xsToString(xsResult);
			twoHex(macaddr[0], out); out += 2; *out++ = ':';
			twoHex(macaddr[1], out); out += 2; *out++ = ':';
			twoHex(macaddr[2], out); out += 2; *out++ = ':';
			twoHex(macaddr[3], out); out += 2; *out++ = ':';
			twoHex(macaddr[4], out); out += 2; *out++ = ':';
			twoHex(macaddr[5], out); out += 2; *out++ = 0;
		}
	}
	else if (0 == espStrCmp(prop, "SSID")) {
#if ESP32
		wifi_ap_record_t config;

		if (ESP_OK == esp_wifi_sta_get_ap_info(&config))
#else
		struct station_config config;

		if (wifi_station_get_config(&config))
#endif
			xsResult = xsString(config.ssid);
	}
	else if (0 == espStrCmp(prop, "BSSID")) {
#if ESP32
		wifi_ap_record_t config;

		if (ESP_OK == esp_wifi_sta_get_ap_info(&config)) {
#else
		struct station_config config;

		if (wifi_station_get_config(&config)) {
#endif
			char *out;
			xsResult = xsStringBuffer(NULL, 18);
			out = xsToString(xsResult);
			twoHex(config.bssid[0], out); out += 2; *out++ = ':';
			twoHex(config.bssid[1], out); out += 2; *out++ = '.';
			twoHex(config.bssid[2], out); out += 2; *out++ = '.';
			twoHex(config.bssid[3], out); out += 2; *out++ = '.';
			twoHex(config.bssid[4], out); out += 2; *out++ = '.';
			twoHex(config.bssid[5], out); out += 2; *out++ = 0;
		}
	}
	else if (0 == espStrCmp(prop, "RSSI")) {
#if ESP32
		wifi_ap_record_t config;

		if (ESP_OK == esp_wifi_sta_get_ap_info(&config))
			xsResult = xsInteger(config.rssi);
#else
		xsResult = xsInteger(wifi_station_get_rssi());
#endif
	}
}
