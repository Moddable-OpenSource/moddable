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

import Backlight from "backlight";
import Button from "button";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

class ButtonFlash {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.button,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 4,
			clock: 5
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 0,
			receive: 17,
			transmit: 16
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 18,
			in: 14,
			out: 15,
			port: 2
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
		led: 8,
		backlight: 22,
		displayDC: 15,
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
			Default: ButtonFlash,
			Flash: ButtonFlash
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
	}
};

export default device;
