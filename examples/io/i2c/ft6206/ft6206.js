/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import I2C from "builtin/i2c";

const none = Object.freeze([]);

class FT6206 extends I2C {
	constructor(dictionary) {
		super({
			...dictionary,
			hz: 600000,
			address: 0x38,
		});

		super.write(Uint8Array.of(0xA8));
		if (17 !== (new Uint8Array(super.read(1)))[0])
			throw new Error("unexpected vendor");

		super.write(Uint8Array.of(0xA3));
		let id = (new Uint8Array(super.read(1)))[0];
		if ((6 !== id) && (100 !== id))
			throw new Error("unexpected chip");

		super.write(Uint8Array.of(0x80, 128));					// touch threshold
		super.write(Uint8Array.of(0x86, 1));					// go to monitor mode when no touch active
	}
	configure(dictionary) {
		if (dictionary.threshold)
			super.write(Uint8Array.of(0x80, dictionary.threshold));

		if (dictionary.monitor)
			super.write(Uint8Array.of(0x86, dictionary.monitor ? 1 : 0));

		if (undefined !== dictionary.flipX)
			this.flipX = dictionary.flipX;

		if (undefined !== dictionary.flipY)
			this.flipY = dictionary.flipY;
	}
	sample() {
		super.write(Uint8Array.of(0x02));						// number of touches
		const length = (new Uint8Array(super.read(1)))[0] & 0x0F;
		if (0 === length)
			return none;

		super.write(Uint8Array.of(0x03));						// read points
		const data = new Uint8Array(super.read(6 * length));	// x, then y
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
			else if (1 === state)								// lift (not always delivered)
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
