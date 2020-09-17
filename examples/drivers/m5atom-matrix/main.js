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

const accelerometerGyro = new MPU6886({
	sda: 25,
	scl: 21
});

const np = new NeoPixel({});

let accelerometerTimerID;

const accelerometer = {
	start(frequency) {
		accelerometer.stop();
		accelerometerTimerID = Timer.repeat(id => {
			accelerometerGyro.configure({
				operation: "accelerometer"
			});
			const sample = accelerometerGyro.sample();
			if (sample) {
				sample.y *= -1;
				sample.z *= -1;
				accelerometer.onreading(sample);
			}
		}, frequency);
	},
	stop() {
		if (undefined !== accelerometerTimerID)
			Timer.clear(accelerometerTimerID);
		accelerometerTimerID = undefined;
	},
	onreading(values) {
		if (button.a.read()) {
			// Change colour of 5x5 matrix depending on orientation
			const x = Math.min(Math.max(values.x, -1), 1) / 2;
			const y = Math.min(Math.max(values.y, -1), 1) / 2;
			const z = Math.min(Math.max(values.z, -1), 1) / 2;
			np.fill(np.makeRGB((128 + x * 255) | 0, (128 + y * 255) | 0, (128 + z * 255) | 0));
		}
		else {
			// random colours
			for (let i = 0, length = np.length; i < length; i++)
				np.setPixel(i, np.makeRGB(255 * Math.random(), 255 * Math.random(), 255 * Math.random()));
		}
		np.update();
	}
}

accelerometer.start(50);
