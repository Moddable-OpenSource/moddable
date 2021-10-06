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

import L3GD20 from "embedded:sensor/Gyroscope/L3GD20";
import Timer from "timer";

const sensor = new L3GD20({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	range: 0b10,	// Config.Range.RANGE_2000_DPS
	enable: 0b0111, // Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features.ENABLE_Z,
	sleep: 0b1000,	// enable
	rate: 0b01,
	bandwidth: 0b11
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace(`Gyro: [${sample.x.toFixed(3)}, ${sample.y.toFixed(3)}, ${sample.z.toFixed(3)}]ºs\n`);
}, 100);

