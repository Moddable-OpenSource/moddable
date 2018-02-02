/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import WiFi from "wifi";

let aps = [];

WiFi.mode = 1;

function scan() {
	WiFi.scan({}, ap => {
		if (ap) {
			if (!aps.find(value => ap.ssid == value)) {
				aps.push(ap.ssid);
				trace(` ${ap.ssid}\n`);
			}
		}
		else
			scan();
	});
}

trace("Scan start\n");
scan();
