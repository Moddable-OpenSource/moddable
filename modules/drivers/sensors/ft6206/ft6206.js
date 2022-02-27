/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";		//@@

class FT6206  {
	#io;

	constructor(options) {
		const {i2c, reset, interrupt, onSample, target} = options;
		const io = this.#io = new i2c.io({
			hz: 100_000,
			address: 0x38,
			...i2c
		});
		io.buffer = new Uint8Array(12);		// two touch points

		if (reset) {
			io.reset = new reset.io({
				...reset
			});

			io.reset.write(0);
			Timer.delay(5);
			io.reset.write(1);
			Timer.delay(150);
		}

		if (17 !== io.readByte(0xA8))
			throw new Error("unexpected vendor");

		const id = io.readByte(0xA3);
		if ((6 !== id) && (100 !== id))
			throw new Error("unexpected chip");

		if (interrupt && onSample) {
			io.interrupt = new interrupt.io({
				...interrupt,
				edge: interrupt.io.Falling,
				onReadable: onSample.bind(this)
			});
		}
		if (target)
			this.target = target;

		this.configure({
			active: false,
			threshold: 128,
			timeout: 10
		});
	}
	close() {
		this.#io?.reset?.close();
		this.#io?.interrupt?.close();
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		const io = this.#io;

		if ("threshold" in options)
			io.writeByte(0x80, options.threshold);

		if ("active" in options)
			io.writeByte(0x86, options.active ? 0 : 1);

		if ("timeout" in options)
			io.writeByte(0x87, options.timeout);

		let value = options.flip;
		if (value) {
			delete io.flipX;
			delete io.flipY;

			if ("h" === value)
				io.flipX = true; 
			else if ("v" === value)
				io.flipY = true; 
			else if ("hv" === value)
				io.flipX = io.flipY = true; 
		}

		value = options.length;
		if (undefined !== value)
			io.length = (1 === value) ? 1 : 2; 

		if ("weight" in options) {
			delete io.weight;
			if (options.weight)
				io.weight = true;
		}

		if ("area" in options) {
			delete io.area;
			if (options.area)
				io.area = true;
		}
	}
	sample() {
		const io = this.#io;

		const length = io.readByte(0x02) & 0x0F;			// number of touches
		if (0 === length)
			return;

		const data = io.readBlock(0x03, io.buffer);
		const result = new Array(length);
		for (let i = 0; i < length; i++) {
			const offset = i * 6;
			const id = data[offset + 2] >> 4;
			if (id && (1 === io.length))
				continue;

			let x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
			let y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];

			if (io.flipX)
				x = 240 - x;

			if (io.flipY)
				y = 320 - y;

			result[i] = {x, y, id};

			if (io.weight)
				result[i].weight = data[offset + 4];

			if (io.area)
				result[i].area = data[offset + 5] >> 4;
		}

		return result;
	}
}

export default FT6206;
