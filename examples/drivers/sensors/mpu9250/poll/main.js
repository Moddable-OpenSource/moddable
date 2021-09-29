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

import MPU9250 from "embedded:sensor/Accelerometer-Gyroscope/MPU9250";
import AK8963 from "embedded:sensor/Magnetometer/AK8963";
import Timer from "timer";

// Configure MPU9250 first to allow passthrough access to onboard AK8963

const sensor = new MPU9250({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	range: 2,			// RANGE_8_G
	gyroRange: 2		// RANGE_1000
});

const magSensor = new AK8963({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

magSensor.configure({
	range: 1,			// 16 bit
	mode: 2			// MODE_CONT_1
});

Timer.repeat(() => {
	const sample = sensor.sample();
	const mag = magSensor.sample();

	trace(`Accel: [${sample.accelerometer.x?.toFixed(2)}, ${sample.accelerometer.y?.toFixed(2)}, ${sample.accelerometer.z?.toFixed(2)}] - `);
	trace(`Gyro: [${sample.gyroscope.x?.toFixed(2)}, ${sample.gyroscope.y?.toFixed(2)}, ${sample.gyroscope.z?.toFixed(2)}]\n`);
	if (undefined !== mag.x)
		trace(`Mag: [${mag.x.toFixed(7)}, ${mag.y.toFixed(7)}, ${mag.z.toFixed(7)}]\n`);
}, 1000);

