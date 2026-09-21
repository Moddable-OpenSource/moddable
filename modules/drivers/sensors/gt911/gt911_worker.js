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

import Time from "time";
import GT911 from "gt911/#sync";

export default function () {
	let sensor;
	let pending;			// latest unsent sample when main is behind
	let outstanding = false;	// true while main has not yet acked

	self.onmessage = function(msg) {
		if (0 === msg) {		// ack
			outstanding = false;
			if (pending) {
				self.postMessage(pending);
				pending = undefined;
				outstanding = true;
			}
		}
		else if ("init" === msg.type) {
			const {config} = msg;
			try {
				sensor = new GT911({
					sensor: {
						...device.i2c.default,
						address: config.address,
						hz: config.hz
					},
					interrupt: (undefined !== config.interruptPin) ? {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: config.interruptPin
					} : undefined,
					onSample() {
						const ticks = Time.ticks;			// captured at the interrupt (or timer) callback
						const points = this.sample();
						if (undefined === points)
							return;
						points.ticks = ticks;

						if (outstanding)
							pending = points;		// drop older, keep freshest
						else {
							outstanding = true;
							self.postMessage(points);
						}
					}
				});
				if (undefined !== config.length)
					sensor.configure({length: config.length});
			}
			catch (e) {
				self.postMessage({error: e.message ?? String(e)});
			}
		}
		else if ("configure" === msg.type) {
			sensor?.configure({length: msg.length});
		}
		else if ("close" === msg.type) {
			sensor?.close();
			sensor = undefined;
		}
	};
}
