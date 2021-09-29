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
    SHTC3 Digital Humidity & Temp Sensor
	https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/2_Humidity_Sensors/Datasheets/Sensirion_Humidity_Sensors_SHTC3_Datasheet.pdf
*/

import Timer from "timer";
import CRC8 from "crc";

const Register = Object.freeze({
//	READ_T_NORMAL:			(0x7866),	// temp 1st
//	READ_T_LOWP:			(0x609C),	// temp 1st, low power
//	READ_RH_NORMAL:			(0x58E0),	// humidity 1st
//	READ_RH_LOWP:			(0x401A),	// humidity 1st, low power
	READ_T_NORMAL_STRETCH:	(0x7CA2),	// temp 1st, clock stretch
	READ_T_LOWP_STRETCH:	(0x6458),	// temp 1st, low power, clock stretch
//	READ_RH_NORMAL_STRETCH:	(0x5C24),	// humidity 1st, clock stretch
//	READ_RH_LOWP_STRETCH:	(0x44DE),	// humidity 1st, low power, clock stretch
	READ_ID:				(0xEFC8),	// read ID
	CMD_WAKE:				(0x3517),	// wake device
	CMD_RESET:				(0x805D),	// reset device
	CMD_SLEEP:				(0xB098),	// sleep device
	
});

const WakeDelayLowPower = 1;
const WakeDelayNormal	= 12;

class SHTC3  {
	#io;
	#wordBuffer;
	#valueBuffer;
	#lowPower = 0;
	#autoSleep = 1;
	#onError;
	#crc;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 1_000_000,		// data sheet says up to 1000 kHz
			address: 0x70,
			...options.sensor
		});

		this.#onError = options.onError;

		const wBuf = this.#wordBuffer = new Uint8Array(2);
		const sBuf = new Uint8Array(3);
		this.#crc = new CRC8(0x31, 0xff);

		this.#writeCommand(Register.CMD_WAKE);
		this.#writeCommand(Register.CMD_RESET);
		this.#writeCommand(Register.READ_ID)
		io.read(sBuf);

		if ((sBuf[2] !== this.#crc.reset().checksum(sBuf.subarray(0,2)))
			|| ((sBuf[0] & 0b00001000) != 0b00001000)
			|| ((sBuf[1] & 0b0111) != 0b0111)) {
			this.#onError?.("unexpected sensor");
			this.#io.close();
			return;
		}

		this.#valueBuffer = new Uint8Array(6);
		if (this.#autoSleep)
			this.#writeCommand(Register.CMD_SLEEP);
	}
	configure(options) {
		if (undefined !== options.lowPower)
			this.#lowPower = options.lowPower;
		if (undefined !== options.autoSleep) {
			this.#autoSleep = options.autoSleep;
			if (this.#autoSleep)
				this.#writeCommand(Register.CMD_SLEEP);
			else {
				this.#writeCommand(Register.CMD_WAKE);
				Timer.delay(1);
			}
		}
	}
	close() {
		this.#writeCommand(Register.CMD_SLEEP);
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const lowPower = this.#lowPower;
		const vBuf = this.#valueBuffer;
		let ret = { thermometer: {}, hygrometer: {} };

		if (this.#autoSleep) {
			this.#writeCommand(Register.CMD_WAKE);
			Timer.delay(1);
		}

		this.#writeCommand(lowPower ? Register.READ_T_LOWP_STRETCH :
				 Register.READ_T_NORMAL_STRETCH);
		Timer.delay(lowPower ? WakeDelayLowPower : WakeDelayNormal);
		this.#io.read(vBuf);

		if (this.#autoSleep)
			this.#writeCommand(Register.CMD_SLEEP);

		if ((vBuf[2] !== this.#crc.reset().checksum(vBuf.subarray(0,2)))
			|| (vBuf[5] !== this.#crc.reset().checksum(vBuf.subarray(3,5))))
			this.#onError?.("bad checksum");
		else {
			ret.thermometer.temperature = ((((vBuf[0] * 256.0) + vBuf[1]) * 175) / 65535) - 45;
			ret.hygrometer.humidity = ((((vBuf[3] * 256.0) + vBuf[4]) * 100) / 65535);
		}
		return ret;
	}
	#writeCommand(command) {
		const io = this.#io;
		const wBuf = this.#wordBuffer;

		wBuf[0] = command >> 8;
		wBuf[1] = command & 0xff;

		io.write(wBuf);
	}
}

export default SHTC3;
