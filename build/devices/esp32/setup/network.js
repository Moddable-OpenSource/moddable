/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import config from "mc/config";
import Time from "time";
import WiFi from "wifi";
import Net from "net";
import SNTP from "sntp";

export default function (done) {
	WiFi.mode = 1;

	if (!config.ssid) {
		trace("No Wi-Fi SSID\n");
		return done();
	}

	let monitor = new WiFi({ssid: config.ssid, password: config.password}, function(msg) {
	   switch (msg) {
		   case "gotIP":
				trace(`IP address ${Net.get("IP")}\n`);

				monitor = monitor.close();
				if (!config.sntp)
					return done();

				new SNTP({host: config.sntp}, function(message, value) {
					if (1 === message) {
						trace("got time\n");
						Time.set(value);
					}
					else if (message < 0)
						trace("can't get time\n");
					else
						return;
					done();
				});
				break;

			case "connect":
				trace(`Wi-Fi connected to "${Net.get("SSID")}"\n`);
				break;

			case "disconnect":
				trace("Wi-Fi disconnected\n");
				break;
		}
	});
}
