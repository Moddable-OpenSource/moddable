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
import LEDrgb from "led/rgb";

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
			data: 8,
			clock: 9
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 0,
			receive: 20,
			transmit: 21
		}
	},
	spi: {
		default: {
			io: "SPI",
			port: 1,
			clock: 6,
			in: 2,
			out: 7
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 1
		}
	},
	pin: {
		button: 9,
		buttonA: 9,
		led: 18,
		led_2: 19,
		led_w: 19,
		led_r: 3,
		led_g: 4,
		led_b: 5
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
			ledB: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.PWM,
						pin: device.pin.led_2,
						invert: 1
					});
				}
			},
			RGB: class {
				constructor(options) {
					return new LEDrgb({
						...options,
						io: device.io.PWM,
						pin: { r: device.pin.led_r, g: device.pin.led_g, b: device.pin.led_b },
						invert: 1
					});
				}
			}
		}
	}
};

export default device;
