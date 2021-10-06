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

import MPU6050 from "embedded:sensor/Accelerometer-Gyroscope/MPU6050";
import config from "mc/config";
const Digital = device.io.Digital;

const sensor = new MPU6050({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin,
	},
	onAlert() {
		let sample = sensor.sample();
		trace(`IRQ Accel: [${sample.accelerometer.x?.toFixed(2)}, ${sample.accelerometer.y?.toFixed(2)}, ${sample.accelerometer.z?.toFixed(2)}] - `);
		trace(`Gyro: [${sample.gyroscope.x?.toFixed(2)}, ${sample.gyroscope.y?.toFixed(2)}, ${sample.gyroscope.z?.toFixed(2)}]\n`);
	}
});

sensor.configure({
	range: 1,			// RANGE_4_G,
	gyroRange: 2,		// RANGE_1000
	lowPassFilter: 6,
	sampleRateDivider: 250
});

