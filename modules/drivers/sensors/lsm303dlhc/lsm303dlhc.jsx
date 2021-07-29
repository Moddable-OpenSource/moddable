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
	LSM303DLHC
	https://www.st.com/resource/en/datasheet/lsm303dlhc.pdf
	Implementation inspired by Adafruit https://github.com/adafruit/Adafruit_HMC5883_Unified

	device DataReady pin is internally pulled up
	magnetometer sample value in Tesla
*/

import Timer from "timer";

const Register = Object.freeze({
	CONFIG_A:	0x00,
	CONFIG_B:	0x01,
	MODE:		0x02,
	DATA_X_MSB:	0x03,
	DATA_X_LSB:	0x04,
	DATA_Z_MSB:	0x05,
	DATA_Z_LSB:	0x06,
	DATA_Y_MSB:	0x07,
	DATA_Y_LSB:	0x08,
	STATUS:		0x09,
	ID_A:		0x0A,
	ID_B:		0x0B,
	ID_C:		0x0C,
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
	INT2CFG: 0x34,
	INT2SRC: 0x35,
	INT2THS: 0x36,
	INT2DUR: 0x37,
	CLICKCFG: 0x38,
	CLICKSRC: 0x39,
	CLICKTHS: 0x3A,
	TIMELIMIT: 0x3B,
	TIMELATENCY: 0x3C,
	TIMEWINDOW: 0x3D
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
		RATE_70:	0b1_1000,
		RATE_220:	0b1_1100
	},
	Range: {
		RANGE_16_G: 0b11,	// +/- 16g
		RANGE_8_G: 0b10,	// +/- 8g
		RANGE_4_G: 0b01,	// +/- 4g
		RANGE_2_G: 0b00		// +/- 2g (default value)
	},
	DataRate: {	 	// register 0x2A (LIS3DH_REG_CTRL_REG1) to set bandwidth
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
	},
	Features: {
		DISABLE:		0,
		ENABLE_X:		0b0001,
		ENABLE_Y:		0b0010,
		ENABLE_Z:		0b0100,
		ENABLE_TEMP:	0b0100_0000,
		ENABLE_ADC:		0b1000_0000,
		ENABLE_LOW_POWER: 0b1000
	},
	Interrupt: {
		ACTIVE_HIGH:	0,
		ACTIVE_LOW:		0b0010,
		I1_CLICK:		0b1000_0000,
		I1_IA1:			0b0100_0000,
		I1_IA2:			0b0010_0000,
		I1_ZYXDA:		0b0001_0000,
		I1_321DA:		0b0000_1000,
		I1_WTM:			0b0000_0100,
		I1_OVERRUN:		0b0000_0010
	}
}, true);

class LSM303DLHC_Mag {
	#io;
	#onAlert;
	#monitor;
//	#values = new Uint8Array(6);
	#values = new ArrayBuffer(6);
	#dataView;
	#rate = Config.Rate.RATE_15;
	#gain = Config.Gain.GAIN_1_3;
	#averaging = 0 << 5;
	#gauss_xy;
	#gauss_z;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x1E,
			...options.sensor
		});

		const ID = new Uint8Array(3);
		io.readBlock(Register.ID_A, ID);
		if (ID[0] !== 72 || ID[1] !== 52 || ID[2] !== 51)
			throw new Error("unexpected sensor");

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

		this.#dataView = new DataView(this.#values);
	}

	configure(options) {
		const io = this.#io;

		if (undefined !== options.rate)
			this.#rate = options.rate;

		io.writeByte(Register.CONFIG_A, this.#rate);

		if (undefined !== options.averaging)
			switch (options.averaging) {
				case 2: this.#averaging = 1 << 5; break;
				case 4: this.#averaging = 2 << 5; break;
				case 8: this.#averaging = 3 << 5; break;
				default: this.#averaging = 0; break;
			}

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
		let ret = {};

		io.readBlock(Register.DATA_X_MSB, this.#values);
		ret.x = (this.#dataView.getInt16(0) / this.#gauss_xy) * 0.0001;
		ret.y = (this.#dataView.getInt16(2) / this.#gauss_xy) * 0.0001;
		ret.z = (this.#dataView.getInt16(4) / this.#gauss_z) * 0.0001;

		return ret;
	}
	#toInt16(high, low) {
		let result = (high << 8) | low;
		if (result > 32767)
			result = result - 65536;
		return result;
	}

}
Object.freeze(LSM303DLHC_Mag.prototype);

class LSM303DLHC_Accel {
	#io;
	#onAlert;
	#monitor;
//	#values = new Int16Array(3);
	#values = new ArrayBuffer(6);
	#dataView;
	#multiplier;
	#rate = Config.DataRate.DATARATE_400_HZ;
	#range = Config.Range.RANGE_4_G;
	#accelEnabled = Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features.ENABLE_Z;
	#lowPower = 0;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x19,
			...options.sensor
		});

		const {alert, onAlert} = options;
		if (alert && onAlert) {
			this.#onAlert = options.onAlert;
			this.#monitor = new alert.io({
				mode: alert.io.InputPullUp,
				...alert,
				edge: alert.io.Falling,
				onReadable: () => this.#onAlert()
			});
			io.writeByte(Register.CTRL6, Config.Interrupt.ACTIVE_LOW);
		}

		this.#dataView = new DataView(this.#values);
	}

	configure(options) {
		const io = this.#io;

		if (undefined !== options.rate)
			this.#rate = parseInt(options.rate);

		if (undefined !== options.range)
			this.#range = parseInt(options.range);

		if (undefined !== options.accel)
			this.#accelEnabled = options.accel & 0b111;

		if (undefined !== options.lowPower)
			this.#lowPower = options.lowPower & Config.Features.ENABLE_LOW_POWER;

		// CTRL1 - Enable axes, normal mode @ rate
		io.writeByte(Register.CTRL1, this.#accelEnabled | this.#lowPower | (this.#rate << 4));

		// CTRL4 - Full Scale, Block data update, big endian, High res
		io.writeByte(Register.CTRL4, 0x88 | (this.#range << 4));

		if (undefined !== options.alert) {
			const alert = options.alert;

			if (undefined !== alert.mode) {
				if (alert.mode === Config.Alert.MOVEMENT) {
					io.writeByte(Register.CTRL2, 0x01);		// HP filter INT1
					io.writeByte(Register.CTRL3, 0x40);		// interrupt to INT1
					io.writeByte(Register.CTRL5, 0x08);		// latch INT1
					io.writeByte(Register.INT1CFG, 0x2a);	// enable xh,yh,zh
				}
			}
			if (undefined !== alert.threshold)
				io.writeByte(Register.INT1THS, alert.threshold & 0x7F);
			if (undefined !== alert.duration)
				io.writeByte(Register.INT1DUR, alert.duration & 0x7F);
		}
		else {
			io.writeByte(Register.CTRL2, 0x00);		// HP filter INT1
			io.writeByte(Register.CTRL3, 0x00);		// interrupt to INT1
		}

		if (this.#range === Config.Range.RANGE_16_G)
			this.#multiplier = 1 / 1365; // different sensitivity at 16g
		else if (this.#range === Config.Range.RANGE_8_G)
			this.#multiplier = 1 / 4096;
		else if (this.#range === Config.Range.RANGE_4_G)
			this.#multiplier = 1 / 8190;
		else
			this.#multiplier = 1 / 16380;
	}

	sample() {
		const io = this.#io;
//		const values = this.#values;
		const multiplier = this.#multiplier;
		let ret = {};

		if (this.#accelEnabled) {
			io.readBlock(Register.OUT_X_L | 0x80, this.#values);
			ret.x = this.#dataView.getInt16(0) * multiplier;
			ret.y = this.#dataView.getInt16(2) * multiplier;
			ret.z = this.#dataView.getInt16(4) * multiplier;
		}

		return ret;
	}
}
Object.freeze(LSM303DLHC_Accel.prototype);

export {LSM303DLHC_Accel as default, LSM303DLHC_Mag, LSM303DLHC_Accel, Config};
