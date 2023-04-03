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
#include "xsHost.h"

#include "string.h"

#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

extern char gSSID[];

void twoHex(uint8_t value, char *out)
{
    static const char *gHex = "0123456789ABCDEF";
    *out++ = c_read8(gHex + (value >> 4));
    *out++ = c_read8(gHex + (value & 15));
}

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

	if (0 == c_strcmp(prop, "IP")) {
		char addrStr[40];
		if (!netif_is_up(netif_list))
			return;
			
		const ip4_addr_t *ip = netif_ip4_addr(netif_list);
		ipaddr_ntoa_r(ip, addrStr, sizeof(addrStr));
		xsResult = xsString(addrStr);
	}
	else if (0 == c_strcmp(prop, "MAC")) {
		uint8_t *macaddr;
		char *out;

		macaddr = netif_list->hwaddr;

		xsResult = xsStringBuffer(NULL, 18);
		out = xsToString(xsResult);
		twoHex(macaddr[0], out); out += 2; *out++ = ':';
		twoHex(macaddr[1], out); out += 2; *out++ = ':';
		twoHex(macaddr[2], out); out += 2; *out++ = ':';
		twoHex(macaddr[3], out); out += 2; *out++ = ':';
		twoHex(macaddr[4], out); out += 2; *out++ = ':';
		twoHex(macaddr[5], out); out += 2; *out++ = 0;

	}
	else if (0 == c_strcmp(prop, "SSID")) {
		xsResult = xsString(gSSID);
	}
//@@ - not yet
/*
	else if (0 == c_strcmp(prop, "BSSID")) {
		char *out;
		char bssid[6];
		wifi_get_ap_bssid(&bssid);
		xsResult = xsStringBuffer(NULL, 18);
		out = xsToString(xsResult);
		twoHex(bssid[0], out); out += 2; *out++ = ':';
		twoHex(bssid[1], out); out += 2; *out++ = ':';
		twoHex(bssid[2], out); out += 2; *out++ = ':';
		twoHex(bssid[3], out); out += 2; *out++ = ':';
		twoHex(bssid[4], out); out += 2; *out++ = ':';
		twoHex(bssid[5], out); out += 2; *out++ = 0;

	}
	else if (0 == c_strcmp(prop, "RSSI")) {
		int rssi = 0;
		wifi_get_rssi(&rssi);
		xsResult = xsInteger(rssi);
	}
	else if (0 == c_strcmp(prop, "CHANNEL")) {
		xsResult = xsInteger(setting.channel);
	}
	else if (0 == c_strcmp(prop, "DNS")) {
		u8_t i = 0;

		xsResult = xsNewArray(0);
		xsVars(1);
		do {
			char addrStr[40];

			const ip_addr_t* addr = dns_getserver(i);

			if (!addr->addr)
				break;

			ipaddr_ntoa_r(addr, addrStr, sizeof(addrStr));
			xsVar(0) = xsStringBuffer(addrStr, c_strlen(addrStr));
			xsSet(xsResult, i++, xsVar(0));
		} while (TRUE);
	}
*/
}
