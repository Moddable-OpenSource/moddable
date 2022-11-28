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
	magnetometer sample value in microtesla
*/

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
		GAIN_1_3:	0b001,
		GAIN_1_9:	0b010,
		GAIN_2_5:	0b011,
		GAIN_4_0:	0b100,
		GAIN_4_7:	0b101,
		GAIN_5_6:	0b110,
		GAIN_8_1:	0b111
	},
	Rate: {
		RATE_0_75:	0b000,
		RATE_1_5:	0b001,
		RATE_3:		0b010,
		RATE_7_5:	0b011,
		RATE_15:	0b100,
		RATE_30:	0b101,
		RATE_70:	0b110
	}
}, true);

class HMC5883 {
	#io;
	#onAlert;
	#onError;
	#monitor;
	#valueBuffer = new Uint8Array(6);
	#rate = Config.Rate.RATE_15;
	#gain = Config.Gain.GAIN_1_3;
	#averaging = 0;
	#gauss_xy;
	#gauss_z;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x1E,
			...options.sensor
		});

		this.#onError = options.onError;

		const ID = new Uint8Array(3);
		io.readBuffer(Register.ID_A, ID);
		if (ID[0] !== 72 || ID[1] !== 52 || ID[2] !== 51) {
			this.#onError("unexpected sensor");
			this.close();
			return;
		}

		const {alert, onAlert} = options;
		if (alert && onAlert) {
			this.#onAlert = options.onAlert;
			this.#monitor = new alert.io({
				mode: alert.io.InputPullUp,
				...alert,
				edge: alert.io.Falling,
				onReadable: () => this.#onAlert()
			});
		}

		// reset to defaults
		io.writeUint8(Register.CONFIG_A, 0b0001_0000);
		io.writeUint8(Register.CONFIG_B, 0b0010_0000);
		io.writeUint8(Register.MODE, 0b0000_0001);
	}

	configure(options) {
		const io = this.#io;

		if (undefined !== options.rate)
			this.#rate = options.rate & 0b111;

		if (undefined !== options.averaging)
			switch (options.averaging) {
				case 2: this.#averaging = 1 << 5; break;
				case 4: this.#averaging = 2 << 5; break;
				case 8: this.#averaging = 3 << 5; break;
				default: this.#averaging = 0; break;
			}

		io.writeUint8(Register.CONFIG_A, this.#averaging | this.#rate << 2);

		if (undefined !== options.mode)
			io.writeUint8(Register.MODE, options.mode & 0b11);

		if (undefined !== options.gain) {
			this.#gain = options.gain & 0b111;
			io.writeUint8(Register.CONFIG_B, this.#gain << 5);
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
	close() {
		this.#monitor?.close();
		this.#monitor = undefined;
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const vBuf = this.#valueBuffer;
		let ret = {};

		// data registers configured in XZY order
		io.readBuffer(Register.DATA_X_MSB, vBuf);
		ret.x = this.#twoC16(vBuf[0], vBuf[1]) / this.#gauss_xy * 0.1;
		ret.z = this.#twoC16(vBuf[1], vBuf[3]) / this.#gauss_z * 0.1;
		ret.y = this.#twoC16(vBuf[4], vBuf[5]) / this.#gauss_xy * 0.1;

		return ret;
	}
	#twoC16(high, low) {
		const val = (high << 8) | low;
		return (val > 32768) ? -(65535 - val + 1) : val;
	}

}
Object.freeze(HMC5883.prototype);

export {HMC5883 as default, HMC5883, Config};
