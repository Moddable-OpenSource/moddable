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

import LEDrgb from "led/rgb";

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 26,
			clock: 27,
			port: 1
		}
	},
	serial: {
		default: {
			io: "Serial",
			receive: 1,
			transmit: 0
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 6,
			in: 4,
			out: 7,
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
		led: 18,
		led_r: 18,
		led_g: 19,
		led_b: 20
	},
	peripheral: {
		led: {
			Default: class {
				constructor(options) {
					return new LEDrgb({
						...options,
						io: device.io.PWM,
						pin: { r: device.pin.led_r, g: device.pin.led_g, b: device.pin.led_b }
					});
				}
			}
		}
	}
};

export default device;
