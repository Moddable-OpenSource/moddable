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

import MPU6050 from "embedded:sensor/Accelerometer-Gyroscope/MPU6050";
import MAG3110 from "embedded:sensor/Magnetometer/MAG3110";

import Backlight from "backlight";
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

class ButtonC {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonC,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 21,
			clock: 22
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
	spi: {
		default: {
			io: "SPI",
			clock: 18,
			in: 19,
			out: 23,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 36
		}
	},
	pin: {
		button: 38,
		buttonA: 39,
		buttonB: 38,
		buttonC: 37,
		led: 15,
		backlight: 32,
		displayDC: 27,
		displaySelect: 14
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
			Default: ButtonB,
			A: ButtonA,
			B: ButtonB,
			C: ButtonC
		},
		led: {
			Default: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 10,
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
						length: 10,
						pin: device.pin.led,
						order: "GRB"
					});
				}
			}
		}
	},
	sensor: {
		IMU: class extends MPU6050 {
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
				sample.accelerometer.y *= -1;
				sample.accelerometer.z *= -1;
				const {x, y, z} = sample.gyroscope;
				sample.gyroscope.x = y * -1;
				sample.gyroscope.y = x * -1;
				sample.gyroscope.z = z * -1;
				return sample;
			}
		},
		Magnetometer: class {
			constructor(options) {
				return new MAG3110({
					...options,
					sensor: {
						...device.i2c.default,
						io: device.io.SMBus
					}
				});
			}
		}
	}
};

export default device;
