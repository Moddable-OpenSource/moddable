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

const none = Object.freeze([]);

class FT6206  {
	#io;

	constructor(options) {
		const io = this.#io = new options.io({
			...options,
			hz: 600000,
			address: 0x38,
		});

		io.write(Uint8Array.of(0xA8));
		if (17 !== (new Uint8Array(io.read(1)))[0])
			throw new Error("unexpected vendor");

		io.write(Uint8Array.of(0xA3));
		let id = (new Uint8Array(io.read(1)))[0];
		if ((6 !== id) && (100 !== id))
			throw new Error("unexpected chip");

		io.write(Uint8Array.of(0x80, 128));					// touch threshold
		io.write(Uint8Array.of(0x86, 1));					// go to monitor mode when no touch active
	}
	configure(options) {
		const io = this.#io;

		if (options.threshold)
			io.write(Uint8Array.of(0x80, options.threshold));

		if (options.monitor)
			io.write(Uint8Array.of(0x86, options.monitor ? 1 : 0));

		if (undefined !== options.flipX)
			this.flipX = options.flipX;

		if (undefined !== options.flipY)
			this.flipY = options.flipY;
	}
	sample() {
		const io = this.#io;

		io.write(Uint8Array.of(0x02));						// number of touches
		const length = (new Uint8Array(io.read(1)))[0] & 0x0F;
		if (0 === length)
			return none;

		io.write(Uint8Array.of(0x03));						// read points
		const data = new Uint8Array(io.read(6 * length));	// x, then y
		const result = new Array(length);
		for (let i = 0; i < length; i++) {
			const offset = i * 6;
			let id = data[offset + 2] >> 4;
			let state = data[offset] >> 6;
			let x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
			let y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];

			if (0 === state)									// down
				state = 1;
			else if (2 === state)								// contact
				state = 2;
			else if ((1 === state) || (3 === state))			// lift (not always delivered)
				state = 3;
			else
				throw new Error("unexpected");

			if (this.flipX)
				x = 240 - x;

			if (this.flipY)
				y = 320 - y;

			result[id] = {x, y, state};
		}

		return result;
	}
}

export default FT6206;
