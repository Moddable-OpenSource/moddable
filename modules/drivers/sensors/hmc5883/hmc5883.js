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
	HMC5883
	https://cdn-shop.adafruit.com/datasheets/HMC5883L_3-Axis_Digital_Compass_IC.pdf
	Implementation inspired by Adafruit https://github.com/adafruit/Adafruit_HMC5883_Unified

	device DataReady pin is internally pulled up
	sample value in Microtesla
*/

import SMBus from "embedded:io/smbus";

const Register = Object.freeze({
	CONFIG_A:	0,
	CONFIG_B:	1,
	MODE:		2,
	DATA_X_MSB:	3,
	DATA_X_LSB:	4,
	DATA_Z_MSB:	5,
	DATA_Z_LSB:	6,
	DATA_Y_MSB:	7,
	DATA_Y_LSB:	8,
	STATUS:		9,
	ID_A:		10,
	ID_B:		11,
	ID_C:		12
});

const Config = Object.freeze({
	Mode: {
		CONTINUOUS: 0,
		SINGLE: 	1,
		IDLE:		2
	},
	Gain: {
		GAIN_1_3:	0b0010_0000,
		GAIN_1_9:	0b0100_0000,
		GAIN_2_5:	0b0110_0000,
		GAIN_4_0:	0b1000_0000,
		GAIN_4_7:	0b1010_0000,
		GAIN_5_6:	0b1100_0000,
		GAIN_8_1:	0b1110_0000
	},
	Rate: {
		RATE_0_75:	0b0_0000,
		RATE_1_5:	0b0_0100,
		RATE_3:		0b0_1000,
		RATE_7_5:	0b0_1100,
		RATE_15:	0b1_0000,
		RATE_30:	0b1_0100,
		RATE_70:	0b1_1000
	}
}, true);

class HMC5883 {
	#io;
	#onAlert;
	#irqMonitor;
	#values = new Uint8Array(6);
	#rate = Config.Rate.RATE_15;
	#gain = Config.Gain.GAIN_1_3;
	#averaging = 0 << 5;
	#gauss_xy;
	#gauss_z;

	constructor(options) {
		const io = this.#io = new SMBus({
			hz: 400_000,
			sendStop: true,
			address: 0x1E,
			...options
		});

		const ID = new Uint8Array(3);
		io.readBlock(Register.ID_A, ID);
		if (ID[0] !== 72 || ID[1] !== 52 || ID[2] !== 51)
			throw new Error("unexpected sensor");

		if (options.alert) {
			const alert = options.alert;

			if (options.onAlert) {
				this.#onAlert = options.onAlert;
				this.#irqMonitor = new alert.io({
					onReadable: () => this.#onAlert(),
					...alert
				});
			}
		}

		this.configure(options);
	}

	configure(options) {
		const io = this.#io;

		if (undefined !== options.rate)
			this.#rate = options.rate;

		if (undefined !== options.averaging)
			switch (options.averaging) {
				case 2: this.#averaging = 1 << 5; break;
				case 4: this.#averaging = 2 << 5; break;
				case 8: this.#averaging = 3 << 5; break;
				default: this.#averaging = 0; break;
			}

		io.writeByte(Register.CONFIG_A, this.#averaging | this.#rate);

		if (undefined !== options.mode)
			io.writeByte(Register.MODE, options.mode);

		if (undefined !== options.gain) {
			this.#gain = options.gain;
			io.writeByte(Register.CONFIG_B, this.#gain);
		}

		switch (this.#gain) {
			case Config.Gain.GAIN_1_3:
				this.#gauss_xy = 1100;
				this.#gauss_z = 980;
				break;
			case Config.Gain.GAIN_1_9:
				this.#gauss_xy = 855;
				this.#gauss_z = 760;
				break;
			case Config.Gain.GAIN_2_5:
				this.#gauss_xy = 670;
				this.#gauss_z = 600;
				break;
			case Config.Gain.GAIN_4_0:
				this.#gauss_xy = 450;
				this.#gauss_z = 400;
				break;
			case Config.Gain.GAIN_4_7:
				this.#gauss_xy = 400;
				this.#gauss_z = 255;
				break;
			case Config.Gain.GAIN_5_6:
				this.#gauss_xy = 330;
				this.#gauss_z = 295;
				break;
			case Config.Gain.GAIN_8_1:
				this.#gauss_xy = 230;
				this.#gauss_z = 205;
				break;
		}
	}
	sample() {
		const io = this.#io;
		const values = this.#values;
		let ret = {};

		io.readBlock(Register.DATA_X_MSB, values);
		ret.x = (this.#toInt16(values[0], values[1]) / this.#gauss_xy * 100).toFixed(2),
		ret.y = (this.#toInt16(values[2], values[3]) / this.#gauss_xy * 100).toFixed(2),
		ret.z = (this.#toInt16(values[4], values[5]) / this.#gauss_z * 100).toFixed(2)

		return ret;
	}
	#toInt16(high, low) {
		let result = (high << 8) | low;
		if (result > 32767)
			result = result - 65536;
		return result;
	}

}
Object.freeze(HMC5883.prototype);

export {HMC5883 as default, HMC5883, Config};
