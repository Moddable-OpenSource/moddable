/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
    SI7020 - temp/humidity
	https://www.te.com/usa-en/product-CAT-HSC0004.html
    Datasheet: https://www.cdiweb.com/datasheets/te/htu21d.pdf

	HTU21d driver works with SI7020
*/

import CRC8 from "crc";
import Timer from "timer";

const Register = Object.freeze({
	TEMP_MEASURE_HOLD: 0xE3,
	HUMID_MEASURE_HOLD: 0xE5,
	TEMP_MEASURE_NOHOLD: 0xF3,
	HUMID_MEASURE_NOHOLD: 0xF5,
	WRITE_USER_REG: 0xE6,
	READ_USER_REG: 0xE7,
	SOFT_RESET: 0xFE
});

class SI7020  {
	#io;
	#byteBuffer = new Uint8Array(1);
	#wordBuffer = new Uint8Array(2);
	#valueBuffer = new Uint8Array(3);
	#crc8;

	constructor(options) {
		const io = this.#io = new (options.io)({
			...options,
			hz: 100_000,
			address: 0x40
		});

		this.#byteBuffer[0] = Register.SOFT_RESET;
		io.write(this.#byteBuffer);

		this.#crc8 = new CRC8(0x31, 0x00);
	}
	configure(options) {
	}
	sample() {
		let ret = {};

		let humid = this.readValue(Register.HUMID_MEASURE_HOLD);
		ret.humidity = (humid * 125.0 / 65536.0) - 6.0;

		let temp = this.readValue(Register.TEMP_MEASURE_HOLD);
		ret.temperature = (temp * 175.72 / 65536.0) - 46.85;

		return ret;
	}
	readValue(command) {
		const io = this.#io;

		this.#byteBuffer[0] = command;
		io.write(this.#byteBuffer);

		Timer.delay(50);		// from datasheet for 14 bit

		io.read(this.#valueBuffer);

		let chk = this.#crc8.checksum(this.#valueBuffer.buffer, 2);
		if (chk !== this.#valueBuffer[2]) {
			trace(`checksum failed: ${chk} vs ${this.#valueBuffer[2]} on ${this.#valueBuffer[0]}, ${this.#valueBuffer[1]}\n`);
			return -1;
		}

		let value = ((this.#valueBuffer[0] << 8) + this.#valueBuffer[1]) & 0xfffc;		return value;		
	}
}

export default SI7020;
