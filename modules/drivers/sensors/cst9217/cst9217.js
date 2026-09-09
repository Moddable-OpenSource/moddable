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

/*
	Hynitron CST9217 capacitive touch controller
	Module: embedded:sensor/Touch/CST9217

	16-bit register protocol (0xD000 family). Use device.io.I2C, not SMBus.
	Default 7-bit address 0x5A. Two simultaneous points.

	Constructor options: sensor, reset, interrupt, onSample, target.
	configure({ length, flip, active, width, height })
*/

import Timer from "timer";

const DEFAULT_ADDRESS = 0x5A;
const DEFAULT_HZ = 400_000;
const MAX_POINTS = 2;
const DATA_LENGTH = MAX_POINTS * 5 + 5;
const ACK = 0xAB;
const EVENT_PRESSED = 0x06;

const CMD_DATA = Object.freeze(Uint8Array.of(0xD0, 0x00).buffer);
const CMD_ACK = Object.freeze(Uint8Array.of(0xD0, 0x00, ACK).buffer);
const CMD_NORMAL = Object.freeze(Uint8Array.of(0xD1, 0x09).buffer);
const CMD_SLEEP = Object.freeze(Uint8Array.of(0xD1, 0x05).buffer);

const identification = Object.freeze({
	model: "CST9217",
	classification: "Touch",
	description: "Hynitron CST9217 capacitive touch"
});

class CST9217 {
	#io;
	#reset;
	#interrupt;
	#buffer = new Uint8Array(DATA_LENGTH);
	#length = MAX_POINTS;
	#flipX = false;
	#flipY = false;
	#active = true;
	#width;
	#height;

	constructor(options) {
		if ("target" in options)
			this.target = options.target;

		const sensor = options.sensor;
		if (!sensor?.io)
			throw new Error("sensor required");

		try {
			if ("reset" in options) {
				const reset = options.reset;
				this.#reset = new reset.io({
					...reset,
					mode: reset.io.Output,
					initialValue: 0
				});
				Timer.delay(10);
				this.#reset.write(1);
				Timer.delay(50);
			}

			this.#io = new sensor.io({
				hz: DEFAULT_HZ,
				address: DEFAULT_ADDRESS,
				...sensor
			});
			this.#io.write(CMD_NORMAL);

			const {interrupt, onSample} = options;
			if (interrupt && onSample) {
				this.#interrupt = new interrupt.io({
					...interrupt,
					edge: interrupt.io.Falling,
					onReadable: onSample.bind(this)
				});
			}

			this.configure({length: MAX_POINTS});
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	close() {
		this.#interrupt?.close();
		this.#interrupt = undefined;
		this.#reset?.close();
		this.#reset = undefined;
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		if ("length" in options) {
			const value = options.length;
			if ((value !== 1) && (value !== 2))
				throw new RangeError("invalid length");
			this.#length = value;
		}

		if ("flip" in options) {
			const value = options.flip;
			this.#flipX = this.#flipY = false;
			if (value === "h")
				this.#flipX = true;
			else if (value === "v")
				this.#flipY = true;
			else if (value === "hv")
				this.#flipX = this.#flipY = true;
			else if (value)
				throw new RangeError(`invalid flip: ${value}`);
		}

		if ("width" in options)
			this.#width = options.width;
		if ("height" in options)
			this.#height = options.height;

		if ("active" in options) {
			if (options.active !== true && options.active !== false)
				throw new RangeError("invalid active");
			this.#active = options.active;
			this.#io.write(this.#active ? CMD_NORMAL : CMD_SLEEP);
		}
	}
	get configuration() {
		let flip = "";
		if (this.#flipX)
			flip += "h";
		if (this.#flipY)
			flip += "v";
		return {
			interrupt: !!this.#interrupt,
			length: this.#length,
			flip,
			active: this.#active
		};
	}
	get identification() {
		return { ...identification };
	}
	sample() {
		const io = this.#io;
		const data = this.#buffer;
		data.set(new Uint8Array(io.writeRead(CMD_DATA, DATA_LENGTH)));
		io.write(CMD_ACK);

		if (data[6] !== ACK)
			return;

		const count = Math.min(data[5] & 0x7f, this.#length);
		if ((count === 0) || (data[0] === ACK) || (data[0] === 0))
			return [];

		const width = this.#width ?? globalThis.screen?.width;
		const height = this.#height ?? globalThis.screen?.height;
		const result = [];
		for (let i = 0; i < count; i++) {
			const offset = (i * 5) + (i ? 2 : 0);
			const event = data[offset] & 0x0f;
			if (event !== EVENT_PRESSED)
				continue;

			let x = (data[offset + 1] << 4) | (data[offset + 3] >> 4);
			let y = (data[offset + 2] << 4) | (data[offset + 3] & 0x0f);
			if (this.#flipX && width)
				x = width - x;
			if (this.#flipY && height)
				y = height - y;

			result.push({
				x,
				y,
				id: (this.#length === 1) ? 0 : (data[offset] >> 4)
			});
		}

		if (result.length === 0)
			return [];
		return result;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default CST9217;
