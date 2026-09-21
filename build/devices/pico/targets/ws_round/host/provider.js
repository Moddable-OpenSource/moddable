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

import QMI8658 from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";

import Backlight from "backlight";

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
		backlight: 25,
		displayDC: 8,
		displaySelect: 9,
		batteryADC: 29
	},
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		}
	},
	sensor: {
		IMU: class extends QMI8658 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.i2c.default,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				[sample.accelerometer.x, sample.accelerometer.y] = [sample.accelerometer.y * -1, sample.accelerometer.x * -1];
				sample.accelerometer.z *= -1;

				[sample.gyroscope.x, sample.gyroscope.y] = [sample.gyroscope.y * -1, sample.gyroscope.x * -1];
				sample.gyroscope.z *= -1;
				return sample;
			}
		}
	}
};

export default device;

