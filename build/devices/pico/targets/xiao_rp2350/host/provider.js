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

import LED from "led/pwm";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 6,
			clock: 7,
			port: 1
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
			clock: 2,
			in: 4,
			out: 3,
			port: 0
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 26
		},
		A0: {
			io: "Analog",
			pin: 26
		},
		A1: {
			io: "Analog",
			pin: 27
		},
		A2: {
			io: "Analog",
			pin: 28
		}
	},
	pin: {
		led: 22,
		ledPower: 23,
		ledYellow: 25,
		displayDC: 0,
		displaySelect: 1
	},
	peripheral: {
		led: {
			Default: class {
				constructor() {
					return new LED({
						io: device.io.PWM,
						pin: device.pin.ledYellow
					});
				}
			},
			RGB: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 1,
							pin: device.pin.led,
							order: "GRB",
							brightness: 32
						},
						power: {
							io: device.io.Digital,
							pin: device.pin.ledPower
						}
					});
				}
			}
		}
	}
};

export default device;
