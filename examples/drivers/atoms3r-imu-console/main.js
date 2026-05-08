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

import Timer from "timer";

const SAMPLE_INTERVAL = 1000;

let acceleration;
let rotationRate;

if (!device?.sensor?.IMU)
	throw new Error("IMU not available");

const imu = new device.sensor.IMU();

Timer.repeat(() => {
	const sample = imu.sample();

	acceleration = sample.accelerometer;
	rotationRate = sample.gyroscope;

	trace("accelerometer ");
	traceVector(acceleration);
	trace(" gyroscope ");
	traceVector(rotationRate);
	trace("\n");
}, SAMPLE_INTERVAL);

function traceVector(values) {
	if (!values) {
		trace("(pending)");
		return;
	}

	trace(`x:${format(values.x)} y:${format(values.y)} z:${format(values.z)}`);
}

function format(value) {
	if (undefined === value)
		return "n/a";
	if (0 <= value)
		return `+${value.toFixed(3)}`;
	return value.toFixed(3);
}
