/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
			data: 2,
			clock: 3
		}
	},
	Serial: {
		default: {
			io: "Serial",
			port: 0,
			receive: 12,
			transmit: 11
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 7,
			in: 5,
			out: 6,
			port: 2
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 1
		}
	},
	pin: {
		button: 28,
		buttonA: 28,
		backlight: 10,
		displayDC: 24,
		displaySelect: 23,
		led: 8
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
					pin: device.pin.backlight
				});
			}
		},
		led: {
			Default: class {
				constructor() {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 1,
							pin: device.pin.led,
							order: "GRB"
						}
					});
				}
			}
		}
	}
};

export default device;
