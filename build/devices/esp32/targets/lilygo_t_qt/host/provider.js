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
	I2C: {
		default: {
			io: "I2C",
			data: 43,
			clock: 44
		}
	},
	Serial: {
		default: {
			io: "Serial",
			port: 0,
			receive: 18,
			transmit: 17
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 36,
			in: 37,
			out: 35,
			port: 2
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 16
		}
	},
	pin: {
		button: 0,
		buttonA: 0,
		buttonB: 47,
		backlight: 10
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			Flash: ButtonA
		},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight,
					invert: 1
				});
			}
		}
	}
};

export default device;
