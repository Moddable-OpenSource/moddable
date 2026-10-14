/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import IMU from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import Backlight from "backlight";
import Button from "button";
import M5PM1 from "m5pm1";

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonA,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonB {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonB,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 9,
			clock: 10
		},
		internal: {
			io: "I2C",
			data: 47,
			clock: 48
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 44,
			transmit: 43
		}
	},
	spi: {
		default: {
			io: "SPI",
			port: 3,
			clock: 40,
			out: 39
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 1
		}
	},
	pin: {
		button: 11,
		buttonA: 11,
		buttonB: 12,
		backlight: 38,
		displayDC: 45,
		displayReset: 21,
		displaySelect: 41,
		IRRX: 42,
		IRTX: 46,
		imuInterrupt: 13,
		powerInterrupt: 13
	},
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB
		},
		Power: class extends M5PM1 {
			constructor(options = {}) {
				super({
					...options,
					...device.i2c.internal,
					io: device.io.SMBus
				});
			}
		}
	},
	sensor: {
		IMU: class extends IMU {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.i2c.internal,
						address: 0x68,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				if (sample.accelerometer) {
					const accelerometerX = sample.accelerometer.x;
					sample.accelerometer.x = -sample.accelerometer.y;
					sample.accelerometer.y = accelerometerX;
					sample.accelerometer.z = -sample.accelerometer.z;
				}
				if (sample.gyroscope) {
					const gyroscopeX = sample.gyroscope.x;
					sample.gyroscope.x = sample.gyroscope.y;
					sample.gyroscope.y = -gyroscopeX;
				}
				return sample;
			}
		}
	}
};

export default device;
