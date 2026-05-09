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

import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import Timer from "timer";

const i2c = device.I2C.internal ?? device.I2C.default;

const sensor = new BMI270({
	sensor: {
		...i2c,
		io: device.io.SMBus
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
