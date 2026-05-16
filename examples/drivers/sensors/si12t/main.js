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

import Touch from "embedded:sensor/Touch/Si12T";
import Timer from "timer";

const sensor = new Touch({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

Timer.repeat(id => {
	const points = sensor.sample();

	trace(`touch: ${points.join(", ")}\n`);
}, 100);

