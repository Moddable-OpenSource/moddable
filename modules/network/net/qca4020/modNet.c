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
#include "xsHost.h"

#define QAPI_USE_WLAN
#include "qapi.h"
#include "qapi_netservices.h"

static void twoHex(uint8_t value, char *out)
{
	static const char *gHex = "0123456789ABCDEF";
	*out++ = c_read8(gHex + (value >> 4));
	*out++ = c_read8(gHex + (value & 15));
}

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));
    uint32_t deviceId = qca4020_wlan_get_active_device();
	uint32_t dataLen;

	if (0 == c_strcmp(prop, "IP")) {
		const char *devName;
 		if (QAPI_OK == qapi_Net_Get_Wlan_Dev_Name(deviceId, &devName)) {
 			uint32_t addr;
			if ((QAPI_OK == qapi_Net_IPv4_Config(devName, QAPI_NET_IPV4CFG_QUERY_E, &addr, NULL, NULL)) && (0 != addr)) {
				xsResult = xsStringBuffer(NULL, 4 * 5);
		        inet_ntop(AF_INET, &addr, xsToString(xsResult), 20);
			}
		}
	}
	else if (0 == c_strcmp(prop, "MAC") || 0 == c_strcmp(prop, "BSSID")) {
		uint8_t buffer[__QAPI_WLAN_MAC_LEN];
		uint16_t param = (prop[0] == 'M') ? __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS : __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID;
		if (QAPI_OK == qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, param, &buffer, &dataLen)) {
			xsResult = xsStringBuffer(NULL, 18);
			char *out = xsToString(xsResult);
			twoHex(buffer[0], out); out += 2; *out++ = ':';
			twoHex(buffer[1], out); out += 2; *out++ = ':';
			twoHex(buffer[2], out); out += 2; *out++ = ':';
			twoHex(buffer[3], out); out += 2; *out++ = ':';
			twoHex(buffer[4], out); out += 2; *out++ = ':';
			twoHex(buffer[5], out); out += 2; *out++ = 0;
		}
	}
	else if (0 == c_strcmp(prop, "SSID")) {
		char ssid[__QAPI_WLAN_MAX_SSID_LENGTH];
		if (QAPI_OK == qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID, ssid, &dataLen))
			xsResult = xsString(ssid);
	}
	else if (0 == c_strcmp(prop, "RSSI")) {
		uint8_t rssi;
		if (QAPI_OK == qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI, &rssi, &dataLen))
			xsResult = xsInteger(rssi);
	}
}
