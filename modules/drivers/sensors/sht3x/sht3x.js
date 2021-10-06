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
    SHT3x Digital Humidity & Temp Sensor
    https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/0_Datasheets/Humidity/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf
*/

import Timer from "timer";
import CRC8 from "crc";

const Register = Object.freeze({
	HIGHREP_STRETCH : (0x2C06), // high repeatability, clock stretch
/*
	MEDREP_STRETCH : (0x2C0D),  // med repeat, clock stretched
	LOWREP_STRETCH : (0x2C10),  // low repeat, clock stretched
	HIGHREP : (0x2400),         // high repeat, no clock stretch
	MEDREP : (0x240B),          // med repeat, no clock stretch
	LOWREP : (0x2416),          // low repeat, no clock stretch
*/
	READSTATUS : (0xF32D),      // read status register
	CLEARSTATUS : (0x3041),     // clear status
	SOFTRESET : (0x30A2),       // soft reset
/*
	HEATEREN : (0x306D),        // heater enable
	HEATERDIS : (0x3066)	      // heater disable
*/
});

class SHT3x  {
	#io;
	#wordBuffer = new Uint8Array(2);
	#statusBuffer = new Uint8Array(3);
	#valueBuffer = new Uint8Array(6);
	#crc;
	#onError;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 1_000_000,		// data sheet says up to 1000 kHz
			address: 0x44,
			...options.sensor
		});

		this.#onError = options.onError;

		this.#writeCommand(Register.SOFTRESET);
		this.#crc = new CRC8(0x31, 0xff);
	}
	configure(options) {
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const vBuf = this.#valueBuffer;
		const status = this.#statusBuffer;
		let ret = { hygrometer: {}, thermometer: {} };

		this.#writeCommand(Register.HIGHREP_STRETCH);
		this.#io.read(vBuf);

		this.#writeCommand(Register.READSTATUS);
		this.#io.read(status);
		if (status[2] !== this.#crc.reset().checksum(status.subarray(0,2)))
			this.#onError?.("bad checksum");

		this.#writeCommand(Register.CLEARSTATUS);

		if (vBuf[2] == this.#crc.reset().checksum(vBuf.subarray(0,2)))
			ret.thermometer.temperature = ((((vBuf[0] * 256.0) + vBuf[1]) * 175) / 65535) - 45;
		if (vBuf[5] == this.#crc.reset().checksum(vBuf.subarray(3,5)))
			ret.hygrometer.humidity = ((((vBuf[3] * 256.0) + vBuf[4]) * 100) / 65535);
		return ret;
	}
	#writeCommand(command) {
		const io = this.#io;
		const wBuf = this.#wordBuffer;

		wBuf[0] = command >> 8;
		wBuf[1] = command & 0xff;

		io.write(wBuf);
		Timer.delay(50);
	}
}

export default SHT3x;
