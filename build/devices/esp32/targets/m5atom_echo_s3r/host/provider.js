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

import Button from "button";

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
			data: 2,
			clock: 1,
		},
		internal: {
			io: "I2C",
			data: 45,
			clock: 0,
		},
	},
	spi: {
		default: {
			io: "SPI",
			port: 3,
			clock: 5,
			out: 6,
		},
	},
	analog: {
		default: {
			io: "Analog",
			pin: 8,
		},
	},
	pin: {
		button: 41
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
	}
};

export default device;
