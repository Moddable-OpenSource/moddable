/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 * Copyright (c) Wilberforce
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
import NeoPixel from "neopixel";
import Timer from "timer";
import MPU6886 from "mpu6886";

const state = {};

state.accelerometerGyro = new MPU6886({
	sda: 25,
	scl: 21
});

const np = new NeoPixel({
	length: 25,
	pin: 27,
	order: "RGB"
});

global.accelerometer = {
	onreading: nop
}

accelerometer.start = function (frequency) {
	accelerometer.stop();
	state.accelerometerTimerID = Timer.repeat(id => {
		state.accelerometerGyro.configure({
			operation: "accelerometer"
		});
		const sample = state.accelerometerGyro.sample();
		if (sample) {
			sample.y *= -1;
			sample.z *= -1;
			accelerometer.onreading(sample);
		}
	}, frequency);
}

accelerometer.stop = function () {
	if (undefined !== state.accelerometerTimerID)
		Timer.clear(state.accelerometerTimerID);
	delete state.accelerometerTimerID;
}

accelerometer.start(50);

// Change colour of 5x5 matrix depending on orientation
accelerometer.onreading = function (values) {
	np.fill(np.makeRGB(127 + values.x * 128, 127 + values.y * 128, 127 + values.z * 128));
	np.update();
}