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
	NXP PCF8523 - Real-time clock/calendar
	https://www.nxp.com/docs/en/data-sheet/PCF8523.pdf
*/

const Register = Object.freeze({
	CTRL1:			0x00,
	TIME:			0x03,
	VALID_BIT:		0x80,
	STOP_BIT:		0b0010_0000
});

class PCF8523 {
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
		catch(e) {
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

		if (reg[0] & Register.VALID_BIT) {
			// if high bit of seconds is set, then time is uncertain
			return undefined;
		}

		if (this.#io.readUint8(Register.CTRL1) & Register.STOP_BIT) {
			return undefined; // disabled
		}

		// yr, mo, day, hr, min, sec
		return Date.UTC(
			bcdToDec(reg[6]) + 2000,
			bcdToDec(reg[5] & 0x1f) - 1,
			bcdToDec(reg[3]),
			bcdToDec(reg[2]),
			bcdToDec(reg[1]),
			bcdToDec(reg[0] & 0x7f) );
	}

	set time(v) {
		let io = this.#io;
		let b = this.#blockBuffer;

		let now = new Date(v);
		let year = now.getUTCFullYear();

		if (year < 2000)
			return undefined;
		
		b[0] = decToBcd(now.getUTCSeconds());
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDate());
		b[4] = decToBcd(now.getUTCDay());
		b[5] = decToBcd(now.getUTCMonth() + 1);
		b[6] = decToBcd(year % 100);

		io.writeBuffer(Register.TIME, b);

		io.writeUint16(Register.CTRL1, 0);			// enable
	}
}

function decToBcd(d) {
	let v = Math.idiv(d, 10);
	v *= 16;
	v += Math.imod(d, 10);
	return v;
}
function bcdToDec(b) {
	let v = Math.idiv(b, 16);
	v *= 10;
	v += Math.imod(b, 16);
	return v;
}

export default PCF8523;

