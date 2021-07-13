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
import { MPU9250, Config } from "embedded:sensor/Accelerometer-Gyroscope/MPU9250";
import { AK8963, Config as MagConfig } from "embedded:sensor/Magnetometer/AK8963";
import Timer from "timer";

// Configure MPU9250 first to allow passthrough access to onboard AK8963

const sensor = new MPU9250({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	range: Config.Accel_Range.RANGE_8_G,
	gyroRange: Config.Gyro_Range.RANGE_1000,
});

const magSensor = new AK8963({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

magSensor.configure({
	range: MagConfig.Range.RANGE_16bit,
	mode: MagConfig.Mode.MODE_CONT_1
});

Timer.repeat(() => {
	const sample = sensor.sample();
	const mag = magSensor.sample();

	trace(`Accel: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}] - `);
	trace(`Gyro: [${sample.gyroX.toFixed(2)}, ${sample.gyroY.toFixed(2)}, ${sample.gyroZ.toFixed(2)}]\n`);
	if (undefined !== mag.x)
		trace(`Mag: [${mag.x.toFixed(7)}, ${mag.y.toFixed(7)}, ${mag.z.toFixed(7)}]\n`);
}, 1000);

