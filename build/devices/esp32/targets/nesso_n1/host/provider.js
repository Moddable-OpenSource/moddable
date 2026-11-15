/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Expander from "NessoN1Expander";
import Touch from "embedded:sensor/Touch/FT6x36";
import IMU from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import Timer from "timer";
import LED from "NessoN1Led";
import Power from "NessoN1Power";

// Allocate shared state at runtime, outside the frozen preloaded module.
const buttonState = Symbol("NessoN1Buttons");
class Button {
	#open = false;
	#input;
	#timer;
	#pressed;
	#notification;

	constructor({onPush, target, onReadable, onWritable, onError} = {}, pin) {
		if (onReadable || onWritable || onError)
			throw new Error;
		if (target)
			this.target = target;
		const state = globalThis[buttonState] ??= {bus: undefined, users: 0};
		state.bus ??= new Expander(0);
		state.users++;
		this.#open = true;
		try {
			this.#input = new state.bus.Digital({pin, mode: state.bus.Digital.InputPullUp});
			this.#input.debounce = 25;
			this.#input.onChanged = onPush;
			this.#pressed = this.#input.read() ^ 1;
			this.#timer = Timer.repeat(() => {
				const pressed = this.#input.read() ^ 1;
				if (pressed === this.#pressed)
					return;
				this.#pressed = pressed;
				this.#notification ??= Timer.set(() => {
					this.#notification = undefined;
					this.#input.onChanged?.call(this);
				}, this.#input.debounce);
			}, 10);
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	configure(options) {
		if (undefined !== options.debounce)
			this.#input.debounce = options.debounce;
	}

	get pressed() {
		return this.#input.read() ^ 1;
	}

	set onChanged(callback) {
		this.#input.onChanged = callback;
	}

	close() {
		if (!this.#open)
			return;
		this.#open = false;
		Timer.clear(this.#timer);
		Timer.clear(this.#notification);
		this.#timer = this.#notification = undefined;
		this.#input?.close();
		this.#input = undefined;
		const state = globalThis[buttonState];
		if (--state.users === 0) {
			state.bus.close();
			state.bus = undefined;
		}
	}
}

class ButtonA extends Button {
	constructor(options) {
		super(options, 0);
	}
}

class ButtonB extends Button {
	constructor(options) {
		super(options, 1);
	}
}

const notes = new Map([
	["C", 4186], ["C#", 4435], ["Db", 4435],
	["D", 4699], ["D#", 4978], ["Eb", 4978],
	["E", 5274], ["F", 5588], ["F#", 5920], ["Gb", 5920],
	["G", 6272], ["G#", 6645], ["Ab", 6645],
	["A", 7040], ["A#", 7459], ["Bb", 7459], ["B", 7902]
]);

class Tone {
	#io = new device.io.PWM({pin: device.pin.buzzer});
	#timer;

	tone(hz, duration) {
		const io = this.#io = new device.io.PWM({from: this.#io, hz});
		io.write(512);

		if (duration) {
			if (this.#timer)
				Timer.schedule(this.#timer, duration);
			else
				this.#timer = Timer.set(() => {
					this.#timer = undefined;
					this.mute();
				}, duration);
		}
		else if (this.#timer) {
			Timer.clear(this.#timer);
			this.#timer = undefined;
		}
	}

	note(note, octave = 4, duration) {
		const frequency = notes.get(note);
		if (!frequency || !Number.isInteger(octave) || (octave < 0) || (octave > 8))
			throw new RangeError;
		this.tone(Math.idiv(frequency, 1 << (8 - octave)), duration);
	}

	mute() {
		Timer.clear(this.#timer);
		this.#timer = undefined;
		this.#io?.write(0);
	}

	close() {
		Timer.clear(this.#timer);
		this.#timer = undefined;
		this.#io?.close();
		this.#io = undefined;
	}
}

const qwiic = {io: "I2C", data: 10, clock: 8};

export default {
	pin: {
		buzzer: 11
	},
	peripheral: {
		Power,
		button: {Default: ButtonA, A: ButtonA, B: ButtonB},
		led: {Default: LED},
		tone: {Default: Tone}
	},
	sensor: {
		Touch: class extends Touch {
			constructor(options) {
				super({
					...options,
					sensor: {...device.i2c.internal, io: device.io.SMBus, address: 0x38}
				});
				this.configure({length: 1});
			}
			sample() {
				const points = super.sample();
				// FT6336 reports portrait coordinates. Piu expects the LCD's landscape coordinates.
				if (points) {
					for (const point of points) {
						const x = point.x;
						point.x = point.y;
						point.y = 134 - x;
					}
				}
				return points;
			}
		},
		IMU: class extends IMU {
			constructor(options) {
				super({
					...options,
					sensor: {...device.i2c.internal, io: device.io.SMBus, address: 0x68}
				});
			}
			sample() {
				const sample = super.sample();
				if (sample.accelerometer) {
					// Screen coordinates: +X right, +Y up, +Z out of the screen.
					const {x, y} = sample.accelerometer;
					sample.accelerometer.x = -y;
					sample.accelerometer.y = x;
				}
				if (sample.gyroscope) {
					const {x, y} = sample.gyroscope;
					sample.gyroscope.x = -y;
					sample.gyroscope.y = x;
				}
				return sample;
			}
		}
	},
	i2c: {
		default: qwiic,
		qwiic,
		internal: qwiic,
		grove: {io: "I2C", data: 5, clock: 4},
	},
	spi: {
		default: {
			io: "SPI",
			clock: 20,
			in: 22,
			out: 21,
			port: 2
		}
	}
};
