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

#include <ifaddrs.h>
#include <netdb.h>
#include <resolv.h>
#include <arpa/inet.h>

#import <CoreWLAN/CoreWLAN.h>

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

	if (0 == c_strcmp(prop, "SSID")) {
		CWInterface* wifi = [[CWWiFiClient sharedWiFiClient] interface];
		NSString *ssid = wifi.ssid;
		if (ssid)
			xsResult = xsString([ssid UTF8String]);
	}
	else if (0 == c_strcmp(prop, "RSSI")) {
		CWInterface* wifi = [[CWWiFiClient sharedWiFiClient] interface];
		NSInteger rssi = wifi.rssiValue;
		xsResult = xsInteger(rssi);
	}
	else if (0 == c_strcmp(prop, "MAC")) {
		CWInterface* wifi = [[CWWiFiClient sharedWiFiClient] interface];
		NSString *mac = wifi.hardwareAddress;
		xsResult = xsString([mac UTF8String]);
	}
	else if (0 == c_strcmp(prop, "BSSID")) {
		CWInterface* wifi = [[CWWiFiClient sharedWiFiClient] interface];
		NSString *bssid = wifi.bssid;
		if (bssid)
			xsResult = xsString([bssid UTF8String]);
	}
	else if (0 == c_strcmp(prop, "IP")) {
		int localhost = 0;
		struct ifaddrs *ifaddr, *ifa;
		if (getifaddrs(&ifaddr) == -1)
			return;
		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL)
				continue;
			if (ifa->ifa_addr->sa_family == AF_INET) {
				struct sockaddr_in* address = (struct sockaddr_in*)ifa->ifa_addr;
				int ip = ntohl(address->sin_addr.s_addr);
				char buffer[22];
				snprintf(buffer, 22, "%u.%u.%u.%u", (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff));
				if (!c_strcmp(buffer, "127.0.0.1"))
					localhost = 1;
				else {
					xsResult = xsString(buffer);
					break;
				}
			}
		}
		if (!xsTest(xsResult) && localhost)
			xsResult = xsString("127.0.0.1");
		freeifaddrs(ifaddr);
	}
	else if (0 == c_strcmp(prop, "DNS")) {
		struct __res_state res;
		int i, index = 0;

		xsVars(1);

		res_ninit(&res);

		xsResult = xsNewArray(0);
		for (i = 0; i < res.nscount; i++) {
			sa_family_t family = res.nsaddr_list[i].sin_family;
			if (AF_INET == family) {
				char str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(res.nsaddr_list[i].sin_addr.s_addr), str, INET_ADDRSTRLEN);
				xsVar(0) = xsString(str);
			} else if (family == AF_INET6) {
				char str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, &(res.nsaddr_list [i].sin_addr.s_addr), str, INET6_ADDRSTRLEN);
				xsVar(0) = xsString(str);
			}
			else
				continue;
			xsSetIndex(xsResult, index++, xsVar(0));
		}

		res_ndestroy(&res);
	}
}

