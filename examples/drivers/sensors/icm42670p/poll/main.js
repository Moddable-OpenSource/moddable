/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import ICM42670P from "embedded:sensor/Accelerometer-Gyroscope/ICM42670P";
import Timer from "timer";

const sensor = device.sensor?.IMU
	? new device.sensor.IMU()
	: new ICM42670P({
		sensor: {
			...(device.i2c.internal ?? device.i2c.default),
			io: device.io.SMBus
		}
	});

sensor.configure({
	accelerometer: {
		scale: 4,
		sampleRate: 100,
		mode: "low noise"
	},
	gyroscope: {
		scale: 500,
		sampleRate: 100,
		mode: "low noise"
	}
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace("Accel: ");
	traceVector(sample.accelerometer);
	trace(" - Gyro: ");
	traceVector(sample.gyroscope);

	if (sample.thermometer)
		trace(` - Temp: ${sample.thermometer.temperature.toFixed(2)} C`);

	trace("\n");
}, 1000);

function traceVector(values) {
	if (!values) {
		trace("[pending]");
		return;
	}

	trace(`[${format(values.x)}, ${format(values.y)}, ${format(values.z)}]`);
}

function format(value) {
	return (undefined === value) ? "n/a" : value.toFixed(3);
}
