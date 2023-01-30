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
	Microchip MCP7940 - Real-time clock/calendar
	https://ww1.microchip.com/downloads/en/DeviceDoc/MCP7940M-Low-Cost%20I2C-RTCC-with-SRAM-20002292C.pdf
*/

const Register = Object.freeze({
	TIME:			0x00,
	DAY:			0x03,
	ENABLED_BIT:	0b0010_0000,
	ENABLE_BIT:		0b1000_0000
});

class MCP7940 {
	#io;
	#blockBuffer = new Uint8Array(7);

	constructor(options) {
		const { clock } = options;
		const io = this.#io = new clock.io({
			hz: 400_000,
			address: 0x6F,
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
		this.#io?.close();
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

		if (0 == (reg[3] & Register.ENABLED_BIT)) {
			// if OSCRUN is low, then time is uncertain
			return undefined;
		}

		// yr, mo, day, hr, min, sec
		return Date.UTC(
			bcdToDec(reg[6]) + 2000,
			bcdToDec(reg[5] & 0x1f) - 1,
			bcdToDec(reg[4]),
			bcdToDec(reg[2] & 0x3f),
			bcdToDec(reg[1]),
			bcdToDec(reg[0] & 0x7f) );
	}

	set time(v) {
		const io = this.#io;
		const b = this.#blockBuffer;

		const now = new Date(v);
		const year = now.getUTCFullYear();

		if (year < 2000)
			return undefined;

		b[0] = decToBcd(now.getUTCSeconds()) | Register.ENABLE_BIT;		// make sure oscillator is enabled
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDay());
		b[4] = decToBcd(now.getUTCDate());
		b[5] = decToBcd(now.getUTCMonth() + 1);
		b[6] = decToBcd(year % 100);

		// stop the oscillator
		let ST = io.readUint8(Register.TIME);
		if (ST & Register.ENABLE_BIT) {
			ST &= ~Register.ENABLE_BIT;
			io.writeUint8(Register.TIME, ST);
		}

		io.writeBuffer(Register.TIME, b);		// enable is included
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

export default MCP7940;
