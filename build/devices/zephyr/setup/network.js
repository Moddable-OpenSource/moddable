/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

export default function (done) {
	let count = 0;
	for (const name in device.network?.interface) {
		let i = device.network?.interface[name];
		if (("wifi" === i.kind) && !config.ssid) {
			trace("No Wi-Fi SSID\n");
			continue;
		}

		count++;
		const connection = new (i.io)({
			...i,
			onChanged() {
				trace(`device.network.interface.${name} state:  ${this.connection}\n`);
				if (this.connection < 500)
					return;

				trace(` IP address: ${this.address}\n`);
				this.close();

				if (Date.now() > 1672722071_000) {
					if (0 == --count)
						done();
					return;
				}

				const ntp = new device.network.ntp.client.io({
					...device.network.ntp.client
				});

				ntp.getTime((error, value) => {
					if (error)
						trace("can't get time\n");
					else {
						trace("got time\n");
						Time.set(value / 1000);
					}
					ntp.close();
					if (0 == --count)
						done();
				});
			}
		});

		if ("wifi" === i.kind) {
			connection.connect({
				SSID: config.ssid,
				password: config.password
			});
		}
		else
			connection.connect({});
	}

	if (0 === count)
		done();
};
