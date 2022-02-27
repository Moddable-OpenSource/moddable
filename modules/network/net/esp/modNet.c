/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
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
#include "xsHost.h"

#include "string.h"

#if ESP32
	#include "esp_wifi.h"
#else
	#include "user_interface.h"
#endif

#include "lwip/err.h"
#include "lwip/dns.h"

void twoHex(uint8_t value, char *out)
{
	static const char *gHex = "0123456789ABCDEF";
	*out++ = espRead8(gHex + (value >> 4));
	*out++ = espRead8(gHex + (value & 15));
}

#if ESP32
tcpip_adapter_if_t
#else
uint8_t
#endif
getNIF(xsMachine *the)
{
	uint8_t wantsAP = 0, wantsStation = 0;

	if (xsToInteger(xsArgc) > 1) {
		const char *nif = xsToString(xsArg(1));
		wantsAP = 0 == c_strcmp(nif, "ap");
		wantsStation = 0 == c_strcmp(nif, "station");
#if ESP32
		if (0 == c_strcmp(nif, "ethernet"))
			return TCPIP_ADAPTER_IF_ETH;
		if (!wantsAP && !wantsStation) {	// if argument is IP address, find adapter that matches
			ip_addr_t dst;
			if (ipaddr_aton(nif, &dst)) {
				uint8_t ifc;
				dst.u_addr.ip4.addr &= 0x00ffffff;		//@@ this only works for IPv4
				for (ifc = 0; ifc <= TCPIP_ADAPTER_IF_ETH; ifc++) {
					tcpip_adapter_ip_info_t info = {0};
					if ((ESP_OK == tcpip_adapter_get_ip_info(ifc, &info)) && ((info.ip.addr & 0x00ffffff) == dst.u_addr.ip4.addr))
						return ifc;
				}
			}
		}
#endif
	}

#if ESP32
	wifi_mode_t mode;
	esp_err_t err = esp_wifi_get_mode(&mode);

	if (err == ESP_ERR_WIFI_NOT_INIT) {
		if (tcpip_adapter_is_netif_up(TCPIP_ADAPTER_IF_ETH))
			return TCPIP_ADAPTER_IF_ETH;
	} 
	
	if (err != ESP_OK)
		return 255;

	if (wantsStation && ((WIFI_MODE_STA == mode) || (WIFI_MODE_APSTA == mode)))
		return TCPIP_ADAPTER_IF_STA;
	if (wantsAP && ((WIFI_MODE_AP == mode) || (WIFI_MODE_APSTA == mode)))
		return TCPIP_ADAPTER_IF_AP;
	if (wantsAP || wantsStation)
		return 255;
	if (WIFI_MODE_AP == mode)
		return TCPIP_ADAPTER_IF_AP;
	if (WIFI_MODE_STA == mode)
		return TCPIP_ADAPTER_IF_STA;
#else
	uint8 mode = wifi_get_opmode();
	if (wantsStation && ((STATION_MODE == mode) || (STATIONAP_MODE == mode)))
		return STATION_IF;
	if (wantsAP && ((SOFTAP_MODE == mode) || (STATIONAP_MODE == mode)))
		return SOFTAP_IF;
	if (wantsAP || wantsStation)
		return ~0;
	if (SOFTAP_MODE == mode)
		return SOFTAP_IF;
	if (STATION_MODE == mode)
		return STATION_IF;
#endif
	return 255;
}

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

	if (0 == espStrCmp(prop, "IP")) {
#if ESP32
		tcpip_adapter_ip_info_t info = {0};
		tcpip_adapter_if_t nif = getNIF(the);

		if (255 == nif)
			return;

		if ((ESP_OK == tcpip_adapter_get_ip_info(nif, &info)) && info.ip.addr) {
#else
		struct ip_info info;
		uint8_t nif = getNIF(the);

		if (255 == nif)
			return;

		if (wifi_get_ip_info(nif, &info) && (ip4_addr1(&info.ip) || ip4_addr2(&info.ip) || ip4_addr3(&info.ip) || ip4_addr4(&info.ip))) {
#endif
			char addrStr[40];
#if LWIP_IPV4 && LWIP_IPV6
			ip4addr_ntoa_r(&info.ip, addrStr, sizeof(addrStr));
#else
			ipaddr_ntoa_r(&info.ip, addrStr, sizeof(addrStr));
#endif
			xsResult = xsString(addrStr);
		}

	}
	else if (0 == espStrCmp(prop, "MAC")) {
		uint8_t macaddr[6];
#if ESP32
		tcpip_adapter_if_t nif = getNIF(the);
		esp_netif_t *netif = NULL;

		if (TCPIP_ADAPTER_IF_STA == nif)
			netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
		else if (TCPIP_ADAPTER_IF_AP == nif)
			netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
		else if (TCPIP_ADAPTER_IF_ETH == nif)
			netif = esp_netif_get_handle_from_ifkey("ETH_DEF");

		if (!netif)
			return;

		if (ESP_OK == esp_netif_get_mac(netif, macaddr))
#else
		if (wifi_get_macaddr(getNIF(the), macaddr))
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

		if ((ESP_OK == esp_wifi_sta_get_ap_info(&config)) && config.ssid[0])
#else
		struct station_config config;

		if (wifi_station_get_config(&config) && config.ssid[0])
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
			twoHex(config.bssid[1], out); out += 2; *out++ = ':';
			twoHex(config.bssid[2], out); out += 2; *out++ = ':';
			twoHex(config.bssid[3], out); out += 2; *out++ = ':';
			twoHex(config.bssid[4], out); out += 2; *out++ = ':';
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
	else if (0 == espStrCmp(prop, "CHANNEL")) {
#if ESP32
		uint8_t primary;
		wifi_second_chan_t second;

		if (ESP_OK == esp_wifi_get_channel(&primary, &second))
			xsResult = xsInteger(primary);
#else
		xsResult = xsInteger(wifi_get_channel());
#endif
	}
	else if (0 == espStrCmp(prop, "DNS")) {
		u8_t i = 0;

		xsResult = xsNewArray(0);
		xsVars(1);
		do {
			char addrStr[40];
#if ESP32
			const ip_addr_t* addr = dns_getserver(i);
#else
			const ip_addr_t address = dns_getserver(i);
			const ip_addr_t *addr = &address;
#endif
#if LWIP_IPV4 && LWIP_IPV6
			if (!addr->u_addr.ip4.addr)
#else
			if (!addr->addr)
#endif
				break;

			ipaddr_ntoa_r(addr, addrStr, sizeof(addrStr));
			xsVar(0) = xsStringBuffer(addrStr, c_strlen(addrStr));
			xsSetIndex(xsResult, i++, xsVar(0));
		} while (true);
	}
}
