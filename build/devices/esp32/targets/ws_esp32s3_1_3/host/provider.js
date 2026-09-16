/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 47,
			clock: 48
		}
	},
	Serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 44,
			transmit: 43
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 40,
			in: -1,
			out: 41,
			port: 2
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 6
		}
	},
	pin: {
		batteryADC: 6,
		backlight: 20,
		led: 15,
		imuInterrupt: 46,
		displayDC: 38,
		displaySelect: 39
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
		led: {
			Default: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 1,
							pin: device.pin.led,
							order: "GRB",
							brightness: 32
						}
					});
				}
			}
		}
	},
	sensor: {
		IMU: class extends QMI8658 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.I2C.default,
						address: 0x6b,
						io: device.io.SMBus
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: device.pin.imuInterrupt
					}
				});
			}
			sample() {
				const sample = super.sample();
				sample.accelerometer.x *= -1;
				sample.gyroscope.x *= -1;
				return sample;
			}
		}
	}
};

export default device;
