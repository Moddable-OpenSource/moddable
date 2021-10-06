/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

import LIS3DH from "embedded:sensor/Accelerometer/LIS3DH";
import Timer from "timer";

const sensor = new LIS3DH({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	rate: 2,		// 10 Hz
	enable: 0b111,	// ENABLE_X ENABLE_Y ENABLE_Z,
	lowPower: false
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace(`Motion: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}]\n`);
}, 2000);

