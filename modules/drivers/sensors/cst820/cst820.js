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

export default class CST820 {
	#io;
	#interrupt;
	#buffer = new Uint8Array(7);
	#active = true;

	constructor(options) {
		if ("target" in options)
			this.target = options.target;
		const {sensor, interrupt, onSample} = options;
		try {
			this.#io = new sensor.io({hz: 100_000, address: 0x15, ...sensor});
			if (interrupt && onSample) {
				this.#interrupt = new interrupt.io({
					...interrupt,
					edge: interrupt.io.Falling,
					onReadable: onSample.bind(this)
				});
			}
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	configure({active = true} = {}) {
		this.#active = !!active;
	}
	get configuration() {
		return {active: this.#active, interrupt: !!this.#interrupt, length: 1};
	}
	get identification() {
		return {model: "CST820", classification: "Touch"};
	}

	sample() {
		if (!this.#active)
			return;

		const bytes = this.#buffer;
		this.#io.readBuffer(0, bytes);
		if (!(bytes[2] & 0x0F) || (bytes[3] >> 6) === 1)
			return [];

		return [{
			x: ((bytes[3] & 15) << 8) | bytes[4],
			y: ((bytes[5] & 15) << 8) | bytes[6],
			id: 0
		}];
	}

	close() {
		try {
			this.#interrupt?.close();
		}
		finally {
			this.#interrupt = undefined;
			this.#io?.close();
			this.#io = undefined;
		}
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
