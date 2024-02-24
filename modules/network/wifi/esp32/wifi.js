/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

/*
	wifi
*/

export default class WiFi @ "xs_wifi_destructor" {
	constructor(dictionary, onNotify) @ "xs_wifi_constructor";
	close() @ "xs_wifi_close";
	set onNotify() @ "xs_wifi_set_onNotify";

	build(dictionary, onNotify) {
		if (dictionary)
			WiFi.connect(dictionary);
		if (onNotify)
			this.onNotify = onNotify;
	}

	static set mode() @ "xs_wifi_set_mode";
	static get mode() @ "xs_wifi_get_mode";
	static scan(dictionary, callback) @ "xs_wifi_scan";
	static connect(dictionary) @ "xs_wifi_connect";		// no arguments to disconnect
	static accessPoint(dictionary) @ "xs_wifi_accessPoint";
	static close() {throw new Error("use WiFi.disconnect()")}		// deprecated
	static disconnect() {WiFi.connect();}

	static Mode = Object.freeze({
		off: -5,
		none: 0,
		station: 1,
		accessPoint: 2
		// 3 reserved for station & accessPoint (WiFi.mode.station | WiFi.mode.accessPoint) 
	});

	static gotIP = "gotIP";
	static lostIP = "lostIP";
	static connected = "connect";
	static disconnected = "disconnect";
}
