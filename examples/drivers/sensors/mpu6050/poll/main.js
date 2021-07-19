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

import device from "embedded:provider/builtin";
import { MPU6050, Config } from "embedded:sensor/Accelerometer-Gyroscope/MPU6050";
import Timer from "timer";

const sensor = new MPU6050({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	range: Config.Accel_Range.RANGE_8_G,
	gyroRange: Config.Gyro_Range.RANGE_1000
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace(`Accel: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}] - `);
	trace(`Gyro: [${sample.gyroX.toFixed(2)}, ${sample.gyroY.toFixed(2)}, ${sample.gyroZ.toFixed(2)}]\n`);
}, 2000);

