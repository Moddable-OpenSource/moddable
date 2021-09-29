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
import config from "mc/config";
const Digital = device.io.Digital;

let magSensor;

const sensor = new MPU9250({
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
		if (magSensor) {
			const mag = magSensor.sample();
			if (mag.x)
				trace(`Mag: [${mag.x.toFixed(7)}, ${mag.y.toFixed(7)}, ${mag.z.toFixed(7)}]\n`);
		}
	}
});

sensor.configure({
	range: 1,			// RANGE_4_G
	gyroRange: 2,		// RANGE_1000
	lowPassFilter: 6,
	sampleRateDivider: 250
});

magSensor = new AK8963({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

magSensor.configure({
	range: 0,		// 14 bit
	mode: 2			// MODE_CONT_1
});

