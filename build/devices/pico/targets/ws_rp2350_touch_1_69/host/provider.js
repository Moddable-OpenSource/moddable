/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/Touch/CST816S";
import RTC from "embedded:RTC/PCF85063";
import IMU from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";

import Backlight from "backlight";
import Button from "button";

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

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 6,
			clock: 7,
			port: 0
		},
		external: {
			io: "I2C",
			data: 26,
			clock: 27,
			port: 1
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 10,
			in: 12,
			out: 11,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 29
		}
	},
	pin: {
		button: 14,
		buttonA: 14,
		backlight: 25,
		displayDC: 8,
		displaySelect: 9,
		batteryADC: 29
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
	},
	sensor: {
		Touch: class {
			constructor(options) {
				const result = new Touch({
					...options,
					sensor: {
						...device.i2c.default,
						io: device.io.SMBus
					},
					reset: {
						io: device.io.Digital,
						mode: device.io.Digital.Output,
						pin: 22
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: 21
					}
				});
				result.configure({ });
				return result;
			}
		},
		IMU: class extends IMU {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.i2c.default,
						address: 0x6b,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				[sample.accelerometer.x, sample.accelerometer.y] = [sample.accelerometer.y, sample.accelerometer.x ];
				return sample;
			}
		}
	}
};

device.rtc = {
	io: RTC,
	clock: {
		...device.i2c.default,
		io: "SMBus"
	}
};

export default device;

