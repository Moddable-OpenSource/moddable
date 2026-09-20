/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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
			clock: 5,
			port: 0
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
			pin: 40
		},
		A0: {
			io: "Analog",
			pin: 40
		},
		A1: {
			io: "Analog",
			pin: 41
		},
		A2: {
			io: "Analog",
			pin: 42
		}
	},
	pin: {
		led: 25,
		button: 45,
		buttonA: 45
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
						invert: true
					});
				}
			}
		}
	}
};

export default device;

