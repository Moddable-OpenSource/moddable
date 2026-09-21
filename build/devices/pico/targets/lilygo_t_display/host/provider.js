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
import LED from "led/pwm";

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
			data: 4,
			clock: 5,
			port: 0
		}
	},
	serial: {
		default: {
			io: "Serial",
			receive: 13,
			transmit: 12
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 14,
			in: 12,
			out: 15,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 26
		}
	},
	pin: {
		button: 6,
		buttonA: 6,
		buttonB: 7,
		led: 25,
		backlight: 4,
		lcdPower: 22
	},
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.PWM,
						pin: device.pin.led
					});
				}
			}
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
		}
	}
};

export default device;

