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

import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";

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

class ButtonFlash {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonFlash,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 2,
			clock: 1
		},
		internal: {
			io: "I2C",
			data: 38,
			clock: 39
		}
	},
	SPI: {
		default: {
			io: "SPI",
			port: 3,
			clock: 17,
			out: 21
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 8
		}
	},
	pin: {
		button: 41,
		buttonA: 41,
		buttonFlash: 0,
		backlight: 16,
		displayDC: 33,
		displaySelect: 15,
		IRTX: 4
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
			Default: ButtonFlash,
			A: ButtonA,
			Flash: ButtonFlash
		},
	},
	sensor: {
		IMU: class extends MPU6886 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.I2C.internal,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				sample.accelerometer.x *= -1;
				sample.accelerometer.y *= -1;
				sample.gyroscope.x *= -1;
				return sample;
			}
		}
	}
};

export default device;
