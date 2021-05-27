/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import Scanner from "wifiscanner";
import WiFi from "wifi";

WiFi.mode = 1;

trace("Scan start\n");
new Scanner({
	onFound(ap) {
		if (!ap.ssid)
			return false;

		trace(`+ ${ap.ssid}\n`);
	},
	onLost(ssid) {
		trace(`- ${ssid}\n`);
	},
	max: 64,
	scanOptions: {
		hidden: true,
	},
});
