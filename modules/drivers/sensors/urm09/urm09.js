/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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
	ONE_SHOT:		0b0,		// Config bit 7
	CONTINUOUS:		0b1,		// Config bit 7

	RANGE_500CM:	0b0010,		// Range is bits 5-4
	RANGE_300CM:	0b0001,
	RANGE_150CM:	0b0000,
});

const READ_DELAY = Object.freeze([ 20, 30, 40 ]);

const CMD_READ_ONCE = 0b01; 	// write to command register if CONFIG_ONE_SHOT

class URM09 {
	#io;
	#mode = Config.ONE_SHOT;
	#range = Config.RANGE_500CM;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000, 
			address: 0x11,
			...options.sensor
		});

		if (io.readUint8(Register.PRODUCT_ID) !== 0x01)
			throw new Error("unexpected sensor");
	}
	configure(options) {
		if (undefined !== options.range)
			this.#range = options.range & 0b11;

		if (undefined !== options.mode)
			this.#mode = options.mode & 0b1;

		const config = (this.#range << 4) | (this.#mode << 7);
//		this.#io.writeUint8(Register.CONFIG, this.#mode | this.range);
		this.#io.writeUint8(Register.CONFIG, config);
	}
	sample() {
		const io = this.#io;
		let ret = { proximity: {}, thermometer: {}};

		if ((this.#mode & Config.CONTINUOUS) !== Config.CONTINUOUS) {
			io.writeUint8(Register.COMMAND, CMD_READ_ONCE);
			Timer.delay(READ_DELAY[this.#range]);
		}
		switch (this.#range) {
			case Config.RANGE_500CM: ret.proximity.max = 500; break;
			case Config.RANGE_300CM: ret.proximity.max = 300; break;
			case Config.RANGE_150CM: ret.proximity.max = 150; break;
		}

		ret.proximity.distance = io.readUint16(Register.DISTANCE_MSB, true);
		if (ret.proximity.distance == 0xffff)
			ret.proximity.distance = null;
		if (ret.proximity.distance <= ret.proximity.max)
			ret.proximity.near = true;
		ret.thermometer.temperature = io.readUint16(Register.TEMP_MSB, true) / 10;

		return ret;
	}
}
Object.freeze(URM09.prototype);

export {URM09 as default, URM09, Config};
