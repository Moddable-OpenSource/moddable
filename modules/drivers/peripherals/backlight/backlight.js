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

import Timer from "timer";

class Backlight {
	#io;
	#brightness = 0;

	constructor(options) {
		this.#io = new options.io(options);
		if (options.invert)
			this.#io.invert = true;
	}
	close() {
		Timer.clear(this.#io?.timer);
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		if ("fade" in options)
			this.#io.fade = options.fade;
	}
	set brightness(value) {
		const io = this.#io;
		const range = (1 << io.resolution) - 1;
		if (value <= 0)
			value = 0;
		else if (value >= 1)
			value = range;
		else
			value *= range;

		if (io.fade) {
			Timer.clear(io.timer);
			delete io.timer;

			const from = this.#brightness * range, to = value;
			const start = Date.now();
			io.timer = Timer.repeat(() => {
				const fraction = Math.min(1, (Date.now() - start) / io.fade);
				const value = from + ((to - from) * (fraction * fraction));
				io.write(io.invert ? range - value : value);
				if (fraction >= 1) {
					Timer.clear(io.timer);
					delete io.timer;
				}
			}, 20);
		}
		else
			io.write(io.invert ? range - value : value);
		this.#brightness = value / range;
	}
	get brightness() {
		return this.#brightness;
	}
	write(value) {
		this.brightness = value / 100;
	}
}

export default Backlight;
