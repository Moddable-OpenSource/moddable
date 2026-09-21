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
import LEDrgb from "led/rgb";

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 4,
			clock: 5
		},
		internal: {
			io: "I2C",
			data: 7,
			clock: 27
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 44,
			transmit: 43
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 45,
			in: 46,
			out: 47,
			port: 3
		}
	},
	pin: {
		led: 6,
		ledR: 26,
		ledG: 30,
		ledB: 6,
		displayDC: 43,
		displaySelect: 44
	},
	peripheral: {
		led: {
			Default: class {
				constructor(options) {
					return new LEDrgb({
						...options,
						io: device.io.PWM,
						pin: { r: device.pin.ledR, g: device.pin.ledG, b: device.pin.ledB },
						invert: false
					});
				}
			}
		}
	}
};

export default device;
