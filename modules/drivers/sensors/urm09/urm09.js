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
	URM09 Ultrasonic Sensor
	https://wiki.dfrobot.com/URM09_Ultrasonic_Sensor_(Gravity-I2C)_(V1.0)_SKU_SEN0304
*/

import Timer from "timer";

const Register = Object.freeze({
	ADDRESS:		0x00,
	PRODUCT_ID:		0x01,
	VERSION_NUM:	0x02,
	DISTANCE_MSB:	0x03,
	DISTANCE_LSB:	0x04,
	TEMP_MSB:		0x05,
	TEMP_LSB:		0x06,
	CONFIG:			0x07,
	COMMAND:		0x08
});

const Config = Object.freeze({
	ONE_SHOT:		0b0000_0000,	// Config bit 7
	CONTINUOUS:		0b1000_0000,	// Config bit 7
	RANGE_500CM:	0b0010_0000,	// Range is bits 5-4
	RANGE_300CM:	0b0001_0000,
	RANGE_150CM:	0b0000_0000,
});

const READ_DELAY = Object.freeze([ 20, 30, 40 ]);

const CMD_READ_ONCE = 0b01; 	// write to command register if CONFIG_ONE_SHOT

class URM09 {
	#io;
	#mode = Config.ONE_SHOT;
	#range = Config.RANGE_500CM;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 100_000, 
			address: 0x11,
			...options.sensor
		});

		if (io.readByte(Register.PRODUCT_ID) !== 0x01)
			throw new Error("unexpected sensor");
	}
	configure(options) {
		if (undefined !== options.mode)
			this.#mode = options.mode & 0x1000_0000;

		if (undefined !== options.range)
			this.#range = options.range & 0x0011_0000;

		this.#io.writeByte(Register.CONFIG, this.#mode | this.range);
	}
	sample() {
		const io = this.#io;
		let ret = {};

		if ((this.#mode & Config.CONTINUOUS) !== Config.CONTINUOUS) {
			io.writeByte(Register.COMMAND, CMD_READ_ONCE);
			Timer.delay(READ_DELAY[this.#range >> 4]);
		}
		switch (this.#range) {
			case Config.RANGE_500CM: ret.max = 500; break;
			case Config.RANGE_300CM: ret.max = 300; break;
			case Config.RANGE_150CM: ret.max = 150; break;
		}

		ret.distance = io.readWord(Register.DISTANCE_MSB, true);
		ret.temperature = io.readWord(Register.TEMP_MSB, true) / 10;
		if (ret.distance <= ret.max)
			ret.near = true;

		return ret;
	}
}
Object.freeze(URM09.prototype);

export {URM09 as default, URM09, Config};
