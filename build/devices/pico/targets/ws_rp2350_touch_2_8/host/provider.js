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

import Touch from "embedded:sensor/Touch/CST328";
import RTC from "embedded:RTC/PCF85063";
import QMI8658 from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";
import Backlight from "backlight";

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 6,
			clock: 7,
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
			pin: 26
		}
	},
	pin: {
		backlight: 16
	} ,
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.i2c.default,
						io: device.io.SMBus
					}
				});
			}
		}
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
						pin: 17
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: 18
					}
				});
				result.configure({});
				return result;
			}
		},
		IMU: class extends QMI8658 {
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
				return {
					thermometer: sample.thermometer,
					accelerometer: {
						x: sample.accelerometer.y * -1,
						y: sample.accelerometer.x * -1,
						z: sample.accelerometer.z
					},
					gyroscope: {
						x: sample.gyroscope.y * -1,
						y: sample.gyroscope.x * -1,
						z: sample.gyroscope.z
					}
				}
			}
		}
	}
};

export default device;

