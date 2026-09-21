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

import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";

import Button from "button";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

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
			data: 26,
			clock: 32
		},
		internal: {
			io: "I2C",
			data: 25,
			clock: 21
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 33
		}
	},
	pin: {
		button: 39,
		buttonA: 39,
		led: 27
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		led: {
			Default: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 25,
							pin: device.pin.led,
							order: "GRB",
							brightness: 32
						}
					});
				}
			}
		},
		neopixel: {
			Default: class {
				constructor(options) {
					return new NeoPixel({
						...options,
						length: 25,
						pin: device.pin.led,
						order: "GRB"
					});
				}
			}
		}
	},
	sensor: {
		IMU: class extends MPU6886 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.i2c.internal,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				sample.accelerometer.y *= -1;
				sample.accelerometer.z *= -1;
				[sample.gyroscope.x, sample.gyroscope.y] = [sample.gyroscope.y * -1, sample.gyroscope.x];
				sample.gyroscope.z *= -1;
				return sample;
			}
		}
	}
};

export default device;
