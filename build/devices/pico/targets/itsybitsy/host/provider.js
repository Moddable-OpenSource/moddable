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

import Button from "button";
import LED from "led/pwm";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

class ButtonA {
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
	I2C: {
		default: {
			io: "I2C",
			data: 2,
			clock: 3,
			port: 1
		}
	},
	Serial: {
		default: {
			io: "Serial",
			receive: 1,
			transmit: 0
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 18,
			in: 20,
			out: 19,
			port: 0
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 26
		}
	},
	pin: {
		button: 13,
		buttonA: 13,
		led: 11,
		ledRGB: 17,
		ledPower: 16
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		led: {
			Default: class {
				constructor() {
					return new LED({
						io: device.io.PWM,
						pin: device.pin.led
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
							pin: device.pin.ledRGB,
							order: "GRB",
							brightness: 32
						},
						power: {
							io: device.io.Digital,
							pin: device.pin.ledPower
						}
					});
				}
			}
		}
	}
};

export default device;
