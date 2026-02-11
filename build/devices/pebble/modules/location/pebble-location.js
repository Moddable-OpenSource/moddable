/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Message from "pebble/message";

let busy = false;

class Location  {
	#message;
	#onSample;
	#onError;
	#sample;
	#configure = {};

	constructor(options) {
		if (busy)
			throw new Error("single instance only")

		this.#onSample = options.onSample;
		this.#onError = options.onError;
		this.#message = new Message({
			keys: new Map([
				["LOCATION", 15026]
			]),
			target: this,
			onReadable() {
				const target = this.target;
				const msg = this.read();
				const location = msg.get("LOCATION").split(",");
				if (parseInt(location[0])) {
					const sample = target.#sample = {
						latitude: parseFloat(location[1]),
						longitude: parseFloat(location[2])
					};
					if ("" !== location[3])
						sample.altitude = parseFloat(location[3]);
					if ("" !== location[4])
						sample.accuracy = parseFloat(location[4]);
					if ("" !== location[5])
						sample.altitudeAccuracy = parseFloat(location[5]);
					if ("" !== location[6])
						sample.heading = parseFloat(location[6]);
					if ("" !== location[7])
						sample.speed = parseFloat(location[7]);
					if ("" !== location[8])
						sample.timestamp = parseFloat(location[8]);
					target.#onSample?.();
				}
				else {
					target.#onError?.(new Error("get location failed"));
				}
			},
			onWritable() {
				const target = this.target;

				const configure = target.#configure;
				if (!configure) return;

				const c = ["1"];
				c[1] = (undefined === configure.enableHighAccuracy) ? "" : Number(configure.enableHighAccuracy);
				c[2] = configure.timeout ?? "";
				c[3] = configure.maximumAge ?? "";
				target.#message.write(new Map([["LOCATION", c.join(",")]]));
				target.#configure = undefined;
			}
		});

		busy = true;
	}
	close() {
		busy = false;
		if (!this.#message)
			return;

		this.#message.write(new Map([["LOCATION", "0"]]));
		this.#message.close();
		this.#message = undefined;
	}
	configure(options) {
		this.#configure = options ?? {};
	}
	sample() {
		const sample = this.#sample;
		this.#sample = undefined;
		return sample;
	}
}

export default Location;
