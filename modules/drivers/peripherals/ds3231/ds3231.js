/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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
	DS3231 - Maxim Integrated Extremely Accurate I2C-Integrated RTC
	https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
*/

const Register = Object.freeze({
	TIME:			0x00,
	CONTROL:		0x0e,
	ENABLE_BIT:		0x80,
	CENTURY_BIT:	0x80
});

class DS3231 {
	#io;
	#blockBuffer = new Uint8Array(7);

	constructor(options) {
		const { rtc } = options;
		const io = this.#io = new rtc.io({
			hz: 400_000,	
			address: 0x68,
			...rtc
		});

		try {
			io.readByte(0);
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
	get enabled() {
		return (this.#io.readByte(Register.CONTROL) & Register.ENABLE_BIT) ? false : true; //enabled low
	}
	get time() {
		const reg = this.#blockBuffer;
		if (!this.enabled)
			return undefined;

		this.#io.readBlock(Register.TIME, reg);

		return Date.UTC(
			bcdToDec(reg[6]) + ((reg[5] & Register.CENTURY_BIT) ? 2000 : 1900),
			bcdToDec(reg[5] & 0x1f) - 1,
			bcdToDec(reg[4]),
			bcdToDec(reg[2] & 0x3F),
			bcdToDec(reg[1]),
			bcdToDec(reg[0]));
	}

	set time(v) {
		let io = this.#io;
		let b = this.#blockBuffer;

		let now = new Date(v);
		let year = now.getUTCFullYear();
		b[0] = decToBcd(now.getUTCSeconds());
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDay());
		b[4] = decToBcd(now.getUTCDate());
		b[5] = decToBcd(now.getUTCMonth()+1) | ((year > 2000) ? Register.CENTURY_BIT : 0);
		b[6] = decToBcd(year % 100);

		io.writeBlock(Register.TIME, b);

		let c = io.readByte(Register.CONTROL) & ~Register.ENABLE_BIT;	// enabled LOW
		io.writeByte(Register.CONTROL, c);
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

Object.freeze(DS3231.prototype);
export default DS3231;

