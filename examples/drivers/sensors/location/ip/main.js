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

import IP from "embedded:sensor/Location/IP";

const loc = new IP({
	http: {
		...device.network.https.client,
		host: "ipwho.is"
	},
	onSample() {
		const s = loc.sample();
		if (!s)
			return;
		let line = `${s.latitude.toFixed(6)}, ${s.longitude.toFixed(6)}`;
		if (undefined !== s.accuracy)
			line += `  ±${s.accuracy}m`;
		trace(line + "\n");
	},
	onError(e) {
		trace(`Location error: ${e}\n`);
	}
});

trace(`Location: ${loc.identification.model}\n`);
