/*
 * Copyright (c) 2021-2023 Moddable Tech, Inc.
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
	DS1307 - Maxim Integrated 64 x 8, Serial, I2C Real-Time-Clock
	https://datasheets.maximintegrated.com/en/ds/DS1307.pdf
	https://www.maximintegrated.com/en/products/analog/real-time-clocks/DS1307.html
*/

const Register = Object.freeze({
	TIME:			0x00,
	CONTROL:		0x0e,
	ENABLE_BIT:		0x80,
	CENTURY_BIT:	0x80
});


class DS1307 {
	#io;
	#blockBuffer = new Uint8Array(7);

	constructor(options) {
		const { clock } = options;
		const io = this.#io = new clock.io({
			hz: 400_000,
			address: 0x68,
			...clock
		});

		try {
			io.readUint8(0);
		}
		catch (e) {
			io.close();
			throw e;
		}
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	configure(options) {
	}
	get configuration() {
		return {};
	}
	get time() {
		const io = this.#io;
		const reg = this.#blockBuffer;

		io.readBuffer(Register.TIME, reg);

		if (0 != (reg[0] & Register.ENABLE_BIT)) {
			return undefined;
		}

		// yr, mo, day, hr, min, sec
		return Date.UTC(
			bcdToDec(reg[6]) + 1970,
			bcdToDec(reg[5]) - 1,
			bcdToDec(reg[4]),
			bcdToDec(reg[2] & 0x3F),
			bcdToDec(reg[1]),
			bcdToDec(reg[0] & 0x7f) );
	}
	set time(v) {
		let b = this.#blockBuffer;

		let now = new Date(v);
		let year = now.getUTCFullYear();
		b[0] = decToBcd(now.getUTCSeconds()); // enable is bit 7 of register 0 cleared
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDay() + 1);
		b[4] = decToBcd(now.getUTCDate());
		b[5] = decToBcd(now.getUTCMonth() + 1);
		b[6] = decToBcd(year - 1970);

		this.#io.writeBuffer(Register.TIME, b);
	}
}

function decToBcd(d) {
	let val = d | 0;
	let v = (val / 10) | 0;
	v *= 16;
	v += val % 10;
	return v;
}
function bcdToDec(b) {
	let v = (b / 16) | 0;
	v *= 10;
	v += b % 16;
	return v;
}

Object.freeze(DS1307.prototype);
export default DS1307;
