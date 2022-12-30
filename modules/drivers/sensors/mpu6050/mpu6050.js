/*
 * Copyright (c) 2019-2022 Moddable Tech, Inc.
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
	InvenSense MPU-6050 Accelerometer + Gyro
            Datasheet: 			http://43zrtwysvxb2gf29r5o0athu.wpengine.netdna-cdn.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
            Register Map:       https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
*/

import Timer from "timer";

const REGISTERS = {
    INT_CONFIG: 0x37,
    INT_ENABLE: 0x38,
    ACCEL_XOUT: 0x3B, //big endian
    ACCEL_YOUT: 0x3D,
    ACCEL_ZOUT: 0x3F,
    GYRO_XOUT: 0x43,
    GYRO_YOUT: 0x45,
    GYRO_ZOUT: 0x47,
    PWR_MGMT_1: 0x6B,
    PWR_MGMT_2: 0x6C,
	WHO_AM_I: 0x75,

	SAMPLERATE_DIV: 0x19,
	DLPF_CONFIG: 0x1A,
	GYRO_CONFIG: 0x1B,
	ACCEL_CONFIG: 0x1C
};
Object.freeze(REGISTERS);

const Config = Object.freeze({
	Accel_Range: {
		RANGE_2_G: 0b00,
		RANGE_4_G: 0b01,
		RANGE_8_G: 0b10,
		RANGE_16_G: 0b11	
	},
	Gyro_Range: {
		RANGE_250: 0b00,
		RANGE_500: 0b01,
		RANGE_1000: 0b10,
		RANGE_2000: 0b11
	},
	Alert: {
		DATA_READY: 	1,
		MOVEMENT:		2
	}
});
Object.freeze(Config, true);

const EXPECTED_WHO_AM_I = 0x68;
const GYRO_SCALER = Object.freeze([ 131, 65.5, 32,8, 16.4 ]);	// Datasheet 6.1
const ACCEL_SCALER = Object.freeze([ 16384, 8192, 4096, 2048 ]); // Datasheet 6.2
const Gconversion = 9.80665;

class MPU6050 {
	#io;
	#xlRaw = new ArrayBuffer(6);
	#gyroRaw = new ArrayBuffer(6);
	#xlView;
	#gyroView;
	#range = Config.Accel_Range.RANGE_2_G;
	#gyroRange = Config.Gyro_Range.RANGE_250;
	#onAlert;
	#onError;
	#monitor;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x68,
			...options.sensor
		});

		this.#onError = options.onError;

		this.#xlView = new DataView(this.#xlRaw);
		this.#gyroView = new DataView(this.#gyroRaw);
		const gxlID = io.readUint8(REGISTERS.WHO_AM_I) & 0b01111110;
		if (gxlID != EXPECTED_WHO_AM_I) {
			this.#onError?.("unexpected sensor");
			this.close();
			return;
		}

		// device reset
		io.writeUint8(REGISTERS.PWR_MGMT_1, 0b1000_0000);
		Timer.delay(150);
		io.writeUint8(REGISTERS.PWR_MGMT_1, 0b0000_0001);
		Timer.delay(150);

		const {alert, onAlert} = options;
		if (alert && onAlert) {
			this.#onAlert = options.onAlert;
			this.#monitor = new alert.io({
				mode: alert.io.InputPullUp,
				...alert,
				edge: alert.io.Falling,
				onReadable: () => this.#onAlert()
			});

			// active low, open drain, no latch, i2c bypass
			io.writeUint8(REGISTERS.INT_CONFIG, 0b1101_0010);
			io.writeUint8(REGISTERS.INT_ENABLE, 0b0000_0001);
		}
	}
	configure(options) {
		const io = this.#io;

		if (undefined !== options.range) {
			this.#range = options.range | 0b11;
			io.writeUint8(REGISTERS.ACCEL_CONFIG, this.#range << 3);
		}

		if (undefined !== options.gyroRange) {
			this.#gyroRange = options.gyroRange | 0b11;
			io.writeUint8(REGISTERS.GYRO_CONFIG, this.#gyroRange << 3);
		}

		if (undefined !== options.sampleRateDivider)
			io.writeUint8(REGISTERS.SAMPLERATE_DIV, options.sampleRateDivider & 0xff);

		if (undefined !== options.lowPassFilter)
			io.writeUint8(REGISTERS.DLPF_CONFIG, options.lowPassFilter & 0b111);
	}
	close() {
		this.#monitor?.close();
		this.#monitor = undefined;
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		let ret = { accelerometer: {}, gyroscope: {} };

		io.readBuffer(REGISTERS.ACCEL_XOUT, this.#xlRaw);
		ret.accelerometer.x = this.#xlView.getInt16(0) / ACCEL_SCALER[this.#range];
		ret.accelerometer.y = this.#xlView.getInt16(2) / ACCEL_SCALER[this.#range];
		ret.accelerometer.z = this.#xlView.getInt16(4) / ACCEL_SCALER[this.#range];
		ret.accelerometer.x *= Gconversion;
		ret.accelerometer.y *= Gconversion;
		ret.accelerometer.z *= Gconversion;

		io.readBuffer(REGISTERS.GYRO_XOUT, this.#gyroRaw);
		ret.gyroscope.x = this.#gyroView.getInt16(0) / GYRO_SCALER[this.#gyroRange];
		ret.gyroscope.y = this.#gyroView.getInt16(2) / GYRO_SCALER[this.#gyroRange];
		ret.gyroscope.z = this.#gyroView.getInt16(4) / GYRO_SCALER[this.#gyroRange];

		return ret;
	}
}
Object.freeze(MPU6050.prototype);

export { MPU6050 as default, MPU6050, Config };
