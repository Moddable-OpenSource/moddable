/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Net from "net";

function convert(str)
{
	return str ? str : "";
}


trace(`IP Address: ${convert(Net.get("IP"))}\n`);
trace(`MAC Address: ${convert(Net.get("MAC"))}\n`);
trace(`SSID: ${convert(Net.get("SSID"))}\n`);
trace(`BSSID: ${convert(Net.get("BSSID"))}\n`);
trace(`RSSI: ${convert(Net.get("RSSI"))}\n`);
