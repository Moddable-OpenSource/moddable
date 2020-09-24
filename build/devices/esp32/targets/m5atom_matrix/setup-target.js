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

	globalThis.lights = new NeoPixel({});

	const sensor = new MPU6886;
	globalThis.accelerometer = new Accelerometer(sensor);
	globalThis.gyro = new Gyro(sensor);

	done();
}
