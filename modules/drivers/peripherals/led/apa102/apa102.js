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

// Single APA102 LED. Same on/color/brightness/rainbow API as led/neopixel.
// Options: {io: Digital, data, clock, brightness: 0..255, target}.
import Timer from "timer";

const phases = Object.freeze([
	// red, purple, blue, cyan, green, orange, white, black
	[1, 0, -1, 0, 0, 1, 0, -1],
	[0, 0, 0, 1, 0, 0, 0, -1],
	[0, 1, 0, 0, -1, 0, 1, -1]
], true);

class LEDapa102 {
	#data;
	#clock;
	#brightness = 255;
	#color = {r: 0, g: 0, b: 0};

	constructor(options) {
		if ("target" in options) this.target = options.target;
		const {io: Digital, data, clock} = options;
		try {
			this.#data = new Digital({pin: data, mode: Digital.Output, initialValue: 0});
			this.#clock = new Digital({pin: clock, mode: Digital.Output, initialValue: 0});
			this.brightness = options.brightness ?? 255;
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	close() {
		Timer.clear(this.#data?.timer);
		this.#data?.close();
		this.#clock?.close();
		this.#data = this.#clock = undefined;
	}
	get on() {
		const {r, g, b} = this.#color;
		return (((r << 1) + r + (g << 2) + b) >> 3) / 255;
	}
	set on(value) {
		const gray = clamp(value, 1) * 255;
		this.color = {r: gray, g: gray, b: gray};
	}
	get color() { return {...this.#color}; }
	set color({r, g, b}) {
		this.#color = {r, g, b};
		this.#write();
	}
	get brightness() { return this.#brightness; }
	set brightness(value) {
		this.#brightness = clamp(value, 255);
		this.#write();
	}
	#write() {
		const {r, g, b} = this.#color;
		const scale = this.#brightness / 255;
		// Start frame, full global brightness, scaled BGR, end frame.
		for (const byte of [0, 0, 0, 0, 0xFF, b * scale, g * scale, r * scale, 255, 255, 255, 255]) {
			for (let mask = 128; mask; mask >>= 1) {
				this.#data.write(byte & mask ? 1 : 0);
				this.#clock.write(1);
				this.#clock.write(0);
			}
		}
	}
	rainbow(value) {
		const data = this.#data;
		Timer.clear(data.timer);
		data.timer = undefined;

		if (0 === value) {
			this.on = 0;
			return;
		}

		const step = value?.step ?? 3;
		const rgb = [0, 0, 0], color = {r: 0, g: 0, b: 0};
		let phase = 0;

		data.timer = Timer.repeat(() => {
			let advance;
			for (let i = 0; i < 3; i++) {
				const direction = phases[i][phase];

				rgb[i] += direction * step;
				if (direction) {
					if (rgb[i] >= 255) {
						rgb[i] = 255;
						advance = true;
					}
					else if (rgb[i] <= 0) {
						rgb[i] = 0;
						advance = true;
					}
				}
			}
			if (advance)
				if (++phase >= phases[0].length) phase = 0;

			color.r = rgb[0], color.g = rgb[1], color.b = rgb[2];
			this.color = color;
		}, 33);
	}
	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

function clamp(value, max) {
	value = Number(value);
	return (value > 0) ? ((value < max) ? value : max) : 0;
}

export default LEDapa102;
