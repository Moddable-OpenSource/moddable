/*
 * Copyright (c) 2026 Moddable Tech, Inc. ,Satoshi Tanaka
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

import QMI8658 from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";
import Timer from "timer";

const sensor = new QMI8658({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	ACCEL_SCALE: 8.0 / 32768.0,		// AFS_8G
	GYRO_SCALE: 1024.0 / 32768.0	// GFS_1024DPS
});

Timer.repeat(() => {
	const sample = sensor.sample();
	trace(`Accel: [${sample.accelerometer.x?.toFixed(2)}, ${sample.accelerometer.y?.toFixed(2)}, ${sample.accelerometer.z?.toFixed(2)}] - `);
	trace(`Gyro: [${sample.gyroscope.x?.toFixed(2)}, ${sample.gyroscope.y?.toFixed(2)}, ${sample.gyroscope.z?.toFixed(2)}]\n`);
}, 2000);

