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

import Button from "button";
import LED from "led/digital";

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
	i2c: {
		default: {
			io: "I2C",
			data: 5,
			clock: 4
		}
	},
	serial: {
		default: {
			io: "Serial",
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 14,
			in: 13,
			out: 12,
			port: "HSPI"
		}
	},
	analog: {
		default: {
			io: "Analog"
		}
	},
	pin: {
		button: 0,
		buttonA: 0,
		led: 16,
		ledA: 16,
		ledB: 2
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		led: {
			A: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.Digital,
						pin: device.pin.ledA,
						invert: true
					});
				}
			},
			B: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.Digital,
						pin: device.pin.ledB,
						invert: true
					});
				}
			}
		}
	}
};
device.peripheral.led.Default = device.peripheral.led.A;

export default device;
