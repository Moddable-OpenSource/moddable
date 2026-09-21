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

import Touch from "embedded:sensor/Touch/CST816";
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

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 43,
			clock: 44
		},
		internal: {
			io: "I2C",
			data: 18,
			clock: 17
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 12,
			in: 13,
			out: 11,
			port: 1
		}
	},
	pin: {
		button: 0,
		buttonA: 0,
		buttonB: 14,
		backlight: 38,
        lcdPower: 15
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			Flash: ButtonA
		},
		Power: {
			LCD: class {
				constructor() {
					return new device.io.Digital({
						io: device.io.Digital,
						pin: device.pin.lcdPower,
						mode: device.io.Digital.Output });
					}
			}
		},
		Backlight: class {
			constructor() {
				return new Backlight({
						io: device.io.PWM,
						pin: device.pin.backlight
				});
			}
		}
	}
};

export default device;
