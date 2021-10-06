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
	AHT10 Temperature / Humidity
	https://server4.eca.ir/eshop/AHT10/Aosong_AHT10_en_draft_0c.pdf

	NOTE: This sensor uses address 0x38 which conflicts with the FT6206
			TouchScreen driver for Moddable One and Moddable Two
*/

import Timer from "timer";

const Register = Object.freeze({
	CMD_INIT:	0b1110_0001,
	CMD_MEAS:	0b1010_1100,
	CMD_RESET:	0b1011_1010
});

const Flags = Object.freeze({
//	BUSY:			0b1000_0000,
//	MODE_CMD:		0b0100_0000,
	MODE_CYC:		0b0010_0000,
//	MODE_NOR:		0b0110_0000,
	CAL_ENABLED:	0b0000_1000
});

const RESET_DELAY = 350;
const MEASUREMENT_DELAY = 75;

class AHT10 {
	#io;
	#cmdBuffer = new Uint8Array(3);
	#valueBuffer = new Uint8Array(6);

	constructor(options) {
		const cBuf = this.#cmdBuffer;
		const io = this.#io = new options.sensor.io({
			hz: 400_000, 
			address: 0x38,
			...options.sensor
		});

		cBuf[0] = Register.CMD_INIT;
		cBuf[1] = Flags.MODE_CYC | Flags.CAL_ENABLED;
		cBuf[2] = 0;
		io.write(cBuf, true);

		Timer.delay(RESET_DELAY);
	}
	configure(options) {
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const cBuf = this.#cmdBuffer;
		const vBuf = this.#valueBuffer;
		let ret = { hygrometer: {}, thermometer: {}};

		cBuf[0] = Register.CMD_MEAS;
		cBuf[1] = 0x33;
		cBuf[2] = 0;
		io.write(cBuf, true);

		Timer.delay(MEASUREMENT_DELAY);

		io.read(vBuf, true);

		let h = (vBuf[1] << 12) | (vBuf[2] << 4) | ((vBuf[3] & 0xF0) >> 4);
		let t = ((vBuf[3] & 0x0F) << 16) | (vBuf[4] << 8) | vBuf[5];
		ret.hygrometer.humidity = ((h * 625) >> 16) / 100;
		ret.thermometer.temperature = (((t * 625) >> 15) / 100) - 50;

		return ret;
	}
}
Object.freeze(AHT10.prototype);

export default AHT10;

