/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import GPS from "embedded:sensor/Location/GPS-I2C";
import UBX from "data/location/ubx";
import config from "mc/config";

const gps = new GPS({
	sensor: {
		...device.i2c.default,
		io: device.io.I2C
	},
	protocol: UBX,
	onSample() {
		const s = gps.sample();
		if (!s) {
			trace("GPS: fix lost\n");
			return;
		}
		let line = `${s.latitude.toFixed(6)}, ${s.longitude.toFixed(6)}`;
		if (undefined !== s.altitude)
			line += `  alt=${s.altitude}`;
		if (undefined !== s.accuracy)
			line += `  ±${s.accuracy}`;
		if (undefined !== s.altitudeAccuracy)
			line += `  v±${s.altitudeAccuracy}`;
		if (undefined !== s.speed)
			line += `  ${s.speed.toFixed(2)} m/s`;
		if (undefined !== s.heading)
			line += `  hdg=${s.heading}`;
		trace(line + "\n");
	},
	onError(e) {
		trace(`GPS error: ${e}\n`);
	}
});

trace(`GPS: ${gps.identification.model}  protocol=ubx  i2c\n`);
