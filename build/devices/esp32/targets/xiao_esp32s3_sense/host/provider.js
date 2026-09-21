/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
			port: 1,
			data: 5,
			clock: 6
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 0,
			receive: 44,
			transmit: 43
		}
	},
	spi: {
		default: {
			io: "SPI",
			port: 1,
			clock: 7,
			in: 8,
			out: 9
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 1
		}
	},
	pin: {
		button: 0,
		buttonA: 0,
		led: 21
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
		},
		led: {
			Default: class {
				constructor() {
					return new LED({
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
