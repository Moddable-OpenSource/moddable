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

import Touch from "embedded:sensor/Touch/GT911";
import Backlight from "backlight";
import Button from "button";
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
	I2C: {
		default: {
			io: "I2C",
			data: 4,
			clock: 5
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
			clock: 15,
			in: 16,
			out: 17,
			port: 1
		}
	},
	pin: {
		//@@ button
		button: 0,
		buttonA: 0,
		LED: 48,
		backlight: 14
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
		},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight,
					invert: true
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
							pin: device.pin.LED,
							order: "GRB",
							brightness: 32
						}
					});
				}
			}
		}
	},
	sensor: {
		Touch: class {
			constructor(options) {				
				const address = [0x14, 0x5D].find(address => {
					let result = 1, i;
					try {
						i = new device.io.I2C({
							...device.I2C.default,
							hz: 200_000,
							address
						});
						result = i.write(new ArrayBuffer);		// SMBus write quick (see linux/drivers/i2c/i2c-core-base.c => i2c_default_probe)
					}
					catch {
					}
					i?.close();
					return undefined === result;
				});

				const result = new Touch({
					...options,
					sensor: {
						...device.I2C.default,
						io: device.io.I2C,
						address
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: 38
					}
				});
				result.configure({threshold: 20});
				return result;
			}
		}
	}
};

export default device;
