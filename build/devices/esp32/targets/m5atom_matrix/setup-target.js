/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Timer from "timer";

import M5Button from "m5button";

import NeoPixel from "neopixel";
import MPU6886 from "mpu6886";

class Accelerometer {
	#sensor;
	#timer;

	constructor(sensor) {
		this.#sensor = sensor;
	}
	start(frequency) {
		this.stop();
		this.#timer = Timer.repeat(id => {
			if (!this.onreading)
				return;

			this.#sensor.configure({ operation: "accelerometer" });
			const sample = this.#sensor.sample();
			if (sample)
				this.onreading(sample);
		}, frequency);
	}
	stop() {
		if (undefined !== this.#timer)
			Timer.clear(this.#timer);
		this.#timer = undefined;
	}
}

class Gyro {
	#sensor;
	#timer;

	constructor(sensor) {
		this.#sensor = sensor;
	}
	start(frequency) {
		this.stop();
		this.#timer = Timer.repeat(id => {
			if (!this.onreading)
				return;

			this.#sensor.configure({ operation: "gyroscope" });
			const sample = this.#sensor.sample();
			if (sample)
				this.onreading({x: -sample.y, y: -sample.x, z: sample.z});
		}, frequency);
	}
	stop() {
		if (undefined !== this.#timer)
			Timer.clear(this.#timer);
		this.#timer = undefined;
	}
}

export default function (done) {
	globalThis.button = {
		a: new M5Button(39)
	};

	Object.defineProperty(globalThis, "lights", {
		enumerable: true,
		configurable: true,
		get() {		// instantiate lights on first access
			const value = new NeoPixel({}); 
			Object.defineProperty(globalThis, "lights", {
				enumerable: true,
				configurable: true,
				writable: true,
				value
			});
			return value;
		}
	});

	const sensor = new MPU6886;
	globalThis.accelerometer = new Accelerometer(sensor);
	globalThis.gyro = new Gyro(sensor);

	done();
}
