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
    AM2320 - temp/humidity
    Datasheet: http://www.datasheet-pdf.com/PDF/AM2320-Datasheet-Aosong-952504

*/

import { CRC16 } from "crc";
import Timer from "timer";

const Register = Object.freeze({
	HUMID_MEASURE: 0x00,
	TEMP_MEASURE: 0x02,
	COMMAND_READ: 0x03
});

class AM2320  {
	#io;
	#cmdBuffer = new Uint8Array(3);
	#valueBuffer = new Uint8Array(6);
	#crc;
	#onError;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 100_000,
			address: 0x5C,
			...options.sensor
		});

		this.#onError = options.onError;

		this.#crc = new CRC16(0x8005, 0xFFFF, true, true);
	}
	configure(options) {
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		let ret = { hygrometer: {}, thermometer: {} };

		let humid = this.#readValue(Register.HUMID_MEASURE);
		if (undefined !== humid)
			ret.hygrometer.humidity = (humid / 10.0);

		let temp = this.#readValue(Register.TEMP_MEASURE);
		if (undefined !== temp) {
			if (temp & 0x8000)
				temp = -(temp & 0x7fff);
			ret.thermometer.temperature = (temp / 10.0);
		}

		return ret;
	}
	#readValue(reg) {
		const io = this.#io;
		const cBuf = this.#cmdBuffer;
		const vBuf = this.#valueBuffer;

		// wake device
		io.write(new ArrayBuffer);	// write 0 length buffer
		Timer.delay(10);
		
		// start conversion
		cBuf[0] = Register.COMMAND_READ;
		cBuf[1] = reg;
		cBuf[2] = 0x02;
		io.write(cBuf);

		Timer.delay(2);

		// read data
		io.read(vBuf);

		// check crc
		this.#crc.reset();
		let calcCRC = this.#crc.checksum(vBuf.subarray(0,4));

		if ((vBuf[5] != (calcCRC >> 8)) || (vBuf[4] != (calcCRC & 0xFF)))
			return;

		return (vBuf[2] << 8) | vBuf[3];
	}
}

export default AM2320;
