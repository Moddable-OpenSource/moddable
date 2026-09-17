/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import MAG3110 from "embedded:sensor/Magnetometer/MAG3110";
import Timer from "timer";

const sensor = new MAG3110({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

Timer.repeat(() => {
	const sample = sensor.sample();
	trace(`Magnetic Field: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}] µT\n`);
}, 2000);
