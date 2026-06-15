/*
 * Copyright (c) 2026 Satoshi Tanaka
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

import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import Timer from "timer";

const sensor = new BMI270({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

Timer.repeat(() => {
	const sample = sensor.sample();
	trace(`Accel: [${sample.accelerometer.x?.toFixed(2)}, ${sample.accelerometer.y?.toFixed(2)}, ${sample.accelerometer.z?.toFixed(2)}] - `);
	trace(`Gyro: [${sample.gyroscope.x?.toFixed(2)}, ${sample.gyroscope.y?.toFixed(2)}, ${sample.gyroscope.z?.toFixed(2)}] - `);
	trace(`Temperature: ${sample.thermometer.temperature.toFixed(2)}\n`);
}, 1000);

