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
    ST Microelectronics Gyroscope

    Datasheet: https://www.st.com/resource/en/datasheet/l3gd20h.pdf
	https://www.st.com/resource/en/application_note/an4506-l3gd20h-3axis-digital-output-gyroscope-stmicroelectronics.pdf

*/

import Timer from "timer";

const Register = Object.freeze({
	WHO_AM_I:	0x0F,
	CTRL1: 		0x20,
	CTRL2:		0x21,
	CTRL3:		0x22,
	CTRL4:		0x23,
	CTRL5:		0x24,
	OUT_X_L:	0x28,
	IG_CFG:		0x30,
	IG_SRC:		0x31,
	IG_THS_XH:	0x32,
	IG_THS_YH:	0x34,
	IG_THS_ZH:	0x36,
	IG_DURATION: 0x38
});

const Config = Object.freeze({
	Range: {
		RANGE_245_DPS: 0b00,		// +/- 245 degrees per second
		RANGE_500_DPS: 0b01,		// +/- 500
		RANGE_2000_DPS: 0b10		// +/- 2000
	},
	Features: {
		DISABLE:		0,
		ENABLE_Y:		0b0001,
		ENABLE_X:		0b0010,
		ENABLE_Z:		0b0100,
		SLEEP:			0b1000
	},
	Interrupt: {
		ENABLE:			0b1000_0000,
		ACTIVE_LOW:		0b0010_0000,
		ACTIVE_HIGH:	0,
		DATA_READY:		0b0000_1000
	}
}, true);

const Divider = Object.freeze([0.00875, 0.0175, 0.07]);

class L3GD20 {
	#io;
	#onAlert;
	#onError;
	#monitor;
	#values = new Int16Array(3);
	#range = Config.Range.RANGE_245_DPS;
	#rate =  0;
	#bandwidth = 0;
	#gyroEnabled = Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features.ENABLE_Z;
	#sleep = false;
	#low_odr = false;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x6B,
			...options.sensor
		});

		this.#onError = options.onError;

		const id = io.readUint8(Register.WHO_AM_I);
		if (0xD4 != id && 0xD7 != id) {
            this.#onError?.("unexpected sensor");
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
			io.writeUint8(Register.CTRL6, Config.Interrupt.ACTIVE_LOW);
		}

		this.configure({});
	}

	configure(options) {
		const io = this.#io;

		if (undefined !== options.range)
			this.#range = parseInt(options.range) & 0b11;

		if (undefined !== options.rate)
			this.#rate = parseInt(options.rate) & 0b11;

		if (undefined !== options.lowODR)
			this.#low_odr = options.low_odr ? true : false;

		if (undefined !== options.bandwidth)
			this.#bandwidth = parseInt(options.bandwidth) & 0b11;

		if (undefined !== options.enable)
			this.#gyroEnabled = options.enable & 0b111;

		if (undefined !== options.sleep)
			this.#sleep = options.sleep ? true : false;

		if (undefined !== options.alert) {
			const alert = options.alert;
			let cfg = io.readUint8(Register.IG_CFG);

//			io.writeUint8(Register.CTRL2, 0x01);			// HP filter INT1
			io.writeUint8(Register.CTRL3, 0b1010_0000);	// enable INT1, active low
			cfg |= 0b0110_1010;			// latch INT1, xyz hi
			io.writeUint8(Register.IG_CFG, cfg);

			if (undefined !== alert.threshold) {
				const val = alert.threshold / Divider[this.#range];
				io.writeUint16(Register.IG_THS_XH, val & 0x7F, true);
				io.writeUint16(Register.IG_THS_YH, val & 0x7F, true);
				io.writeUint16(Register.IG_THS_ZH, val & 0x7F, true);
			}
			if (undefined !== alert.duration)
				io.writeUint8(Register.IG_DURATION, alert.duration & 0x7F);
		}
		else {
			io.writeUint8(Register.CTRL2, 0x00);		// HP filter INT1
			io.writeUint8(Register.CTRL3, 0x00);		// interrupt to INT1
		}

		// CTRL4 - Full Scale, Block data update, big endian, High res
		io.writeUint8(Register.CTRL4, this.#range << 4);

// CTRL2, 3, 4, 6, reference, IG_THS, IG_DURATON, IG_CFG, CTRL5, CTRL1
		// CTRL1 - Enable axes, normal mode @ rate
		if (this.#sleep)
			io.writeUint8(Register.CTRL1, 0b1000 | (this.#rate << 6) | (this.#bandwidth << 4));
		else
			io.writeUint8(Register.CTRL1, this.#gyroEnabled | 0b1000 | (this.#rate << 6) | (this.#bandwidth << 4));

		if (this.#low_odr)
			io.writeUint8(Register.LOW_ODR, this.#low_odr ? 1 : 0);
			

	}
	status() {
		return this.#io.readUint8(Register.IG_SRC);
	}
	close() {
		this.#monitor?.close();
		this.#monitor = undefined;
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const div = Divider[this.#range];
		const values = this.#values;
		let ctrl1;
		let ret = {};

		if (this.#gyroEnabled) {
			if (this.#sleep) {
				ctrl1 = io.readUint8(Register.CTRL1);
				io.writeUint8(Register.CTRL1, ctrl1 | this.#gyroEnabled);
				Timer.delay(100);
			}
			io.readBuffer(Register.OUT_X_L | 0x80, values);
			ret.x = values[0] * div;
			ret.y = values[1] * div;
			ret.z = values[2] * div;
			if (this.#sleep)
				io.writeUint8(Register.CTRL1, ctrl1);
		}

		return ret;
	}
}
Object.freeze(L3GD20.prototype);

export default L3GD20;
