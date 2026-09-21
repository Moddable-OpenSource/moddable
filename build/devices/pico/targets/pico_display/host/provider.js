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

import Backlight from "backlight";
import Button from "button";
import LEDrgb from "led/rgb";

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

class ButtonX {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonX,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonY {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonY,
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
			clock: 5,
			port: 0
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 18,
			out: 19,
			port: 0
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 26
		}
	},
	pin: {
		button: 12,
		buttonA: 12,
		buttonB: 13,
		buttonX: 14,
		buttonY: 15,
		led: 6,
		led_r: 6,
		led_g: 7,
		led_b: 8,
		backlight: 14
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			X: ButtonX,
			Y: ButtonY
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

