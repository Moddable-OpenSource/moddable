/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
import WiFi from "embedded:network/interface/wifi";
import SNTP from "sntp";

export default function (done) {
	if (!config.ssid) {
		trace("No Wi-Fi SSID\n");
		return done();
	}

	const w = new WiFi({
		onChanged(/* property */) {
			const connection = this.connection;
			if (connection < 500) {
				if (connection >= 400)
					trace(`Wi-Fi connected to "${this.SSID}"\n`);
				else if (connection <= 200)
					trace(`Wi-Fi disconnected\n`);		//@@ password rejected?
				return;
			}

			trace(`IP address ${this.address}\n`);
			this.close();
			if (!config.sntp || (Date.now() > 1672722071_000))
				return done();

			new SNTP({host: config.sntp}, function(message, value) {
				if (SNTP.time === message) {
					trace(`got unix time ${value} from ${config.sntp}\n`);
					Time.set(value);
				}
				else if (SNTP.error === message)
					trace("can't get time\n");
				else
					return;
				done();
			});
		}
	});

	w.connect(config.password ? {SSID: config.ssid, password: config.password, secure: true} : {SSID: config.ssid});
}
