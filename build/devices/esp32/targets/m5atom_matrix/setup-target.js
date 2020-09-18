import Timer from "timer";

import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

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
			if (sample) {
				sample.y *= -1;
				sample.z *= -1;
				this.onreading(sample);
			}
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
			if (sample) {
				let {x, y, z} = sample;
				const temp = x;
				x = y * -1;
				y = temp * -1;
				z *= -1;
				this.onreading({ x, y, z });
			}
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
		a: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
	};
	button.a.onChanged = nop;
	
	globalThis.lights = new NeoPixel({});

	const sensor = new MPU6886;
	globalThis.accelerometer = new Accelerometer(sensor);
	globalThis.gyro = new Gyro(sensor);

	done();
}

function nop() {
}
