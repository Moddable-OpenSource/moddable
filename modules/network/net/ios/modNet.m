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

//#import <CoreWLAN/CoreWLAN.h>

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

/*
	if (0 == c_strcmp(prop, "SSID")) {
		CWInterface* wifi = [[CWWiFiClient sharedWiFiClient] interface];
		NSString *ssid = wifi.ssid;
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
		xsResult = xsString([bssid UTF8String]);
	}
*/
	xsResult = xsString("Unknown");
}

