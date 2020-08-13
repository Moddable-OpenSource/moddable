/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	Adafruit LIS3DH Accelerometer breakout
		https://www.adafruit.com/product/2809
	Implementation based on Adafruit https://github.com/adafruit/Adafruit_LIS3DH
*/

import SMBus from "pins/smbus";

const Register = {
	STATUS1: 0x07,
	OUTADC1_L: 0x08,
	OUTADC1_H: 0x09,
	OUTADC2_L: 0x0A,
	OUTADC2_H: 0x0B,
	OUTADC3_L: 0x0C,
	OUTADC3_H: 0x0D,
	INTCOUNT: 0x0E,
	WHOAMI: 0x0F,
	TEMPCFG: 0x1F,
	CTRL1: 0x20,
	CTRL2: 0x21,
	CTRL3: 0x22,
	CTRL4: 0x23,
	CTRL5: 0x24,
	CTRL6: 0x25,
	REFERENCE: 0x26,
	STATUS2: 0x27,
	OUT_X_L: 0x28,
	OUT_X_H: 0x29,
	OUT_Y_L: 0x2A,
	OUT_Y_H: 0x2B,
	OUT_Z_L: 0x2C,
	OUT_Z_H: 0x2D,
	FIFOCTRL: 0x2E,
	FIFOSRC: 0x2F,
	INT1CFG: 0x30,
	INT1SRC: 0x31,
	INT1THS: 0x32,
	INT1DUR: 0x33,
	CLICKCFG: 0x38,
	CLICKSRC: 0x39,
	CLICKTHS: 0x3A,
	TIMELIMIT: 0x3B,
	TIMELATENCY: 0x3C,
	TIMEWINDOW: 0x3D,
	ACTTHS: 0x3E,
	ACTDUR: 0x3F
};
Object.freeze(Register);

const Range = {
	RANGE_16_G: 0b11,	// +/- 16g
	RANGE_8_G: 0b10,	// +/- 8g
	RANGE_4_G: 0b01,	// +/- 4g
	RANGE_2_G: 0b00		// +/- 2g (default value)
};
Object.freeze(Range);

// Used with register 0x2A (LIS3DH_REG_CTRL_REG1) to set bandwidth
const DataRate = {
	DATARATE_400_HZ: 0b0111,	//  400Hz 
	DATARATE_200_HZ: 0b0110,	//  200Hz
	DATARATE_100_HZ: 0b0101,	//  100Hz
	DATARATE_50_HZ: 0b0100,		//   50Hz
	DATARATE_25_HZ: 0b0011,		//   25Hz
	DATARATE_10_HZ: 0b0010,		// 10 Hz
	DATARATE_1_HZ: 0b0001,		// 1 Hz
	POWERDOWN: 0,
	LOWPOWER_1K6HZ: 0b1000,
	LOWPOWER_5KHZ: 0b1001,
};
Object.freeze(DataRate);

class Sensor extends SMBus {
	#values = new Int16Array(3);
	#multiplier;
	#rate = DataRate.DATARATE_400_HZ;
	#range = Range.RANGE_4_G;

	constructor(dictionary) {
		super({address: 0x18, throw: false, ...dictionary});

		if (0x33 === this.readByte(Register.WHOAMI))
			;
		else if (0x33 !== this.readByte(Register.WHOAMI))
			throw new Error("unexpected device ID");

		this.configure({});
	}

	configure(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "rate":
					this.#rate = parseInt(dictionary.rate);
					break;

				case "range":
					this.#range = parseInt(dictionary.range);
					break;

				default:
					throw new Error(`invalid property "${property}"`);
			}
		}
		// Enable all axes, normal mode @ rate
		this.writeByte(Register.CTRL1, 0x07 | (this.#rate << 4));

		// High res & BDU enabled
		this.writeByte(Register.CTRL4, 0x88 | (this.#range << 4));

		if (this.#range === Range.RANGE_16_G)
			this.#multiplier = 1 / 1365; // different sensitivity at 16g
		else if (this.#range === Range.RANGE_8_G)
			this.#multiplier = 1 / 4096;
		else if (this.#range === Range.RANGE_4_G)
			this.#multiplier = 1 / 8190;
		else
			this.#multiplier = 1 / 16380;
	}

	sample() {
		const values = this.#values, multiplier = this.#multiplier;

		this.readBlock(Register.OUT_X_L | 0x80, 6, values.buffer);

		return {
			x: values[0] * multiplier,
			y: values[1] * multiplier,
			z: values[2] * multiplier
		};
	}
}
Object.freeze(Sensor.prototype);

export {Sensor as default, Sensor, DataRate, Range};
