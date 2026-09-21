/*
 * Copyright (c) 2023-2026  Moddable Tech, Inc.
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

import SHTC3 from "embedded:sensor/Humidity-Temperature/SHTC3";
import IMU from "embedded:sensor/Accelerometer-Gyroscope/ICM42670P";

import Button from "button";
import LED from "led/pwm";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

class ButtonFlash {
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
			port: "I2C_NUM_0",
			data: 10,
			clock: 8
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 20,
			transmit: 21
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 0
		}
	},
	pin: {
		button: 9,
		buttonA: 9,
		led: 7,
		LEDcolor: 2
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.PWM,
						pin: device.pin.led,
						invert: 1
					});
				}
			},
			RGB: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 1,
							pin: device.pin.LEDcolor,
							order: "GRB",
							brightness: 32
						}
					});
				}
			}
		}
	},
	sensor: {
		Environment: class {
			constructor(options) {
				const sensor = new SHTC3({ sensor: device.i2c.default, address: 0x88 });
				sensor.configure({ lowPower: true, autoSleep: true });
				return sensor;
			}
		},
		IMU: class {
			constructor(options) {
				const sensor = new IMU({
					...options,
					sensor: {
						...device.i2c.default,
						io: device.io.SMBus
					}
				});
				sensor.configure ({
					accelerometer: {
						scale: 4,
						sampleRate: 100,
						mode: "low noise"
					},
					gyroscope: {
						scale: 500,
						sampleRate: 100,
						mode: "low noise"
					}
				});
				return sensor;
			}
		}
	}
};

export default device;
