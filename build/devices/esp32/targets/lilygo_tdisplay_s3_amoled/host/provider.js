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

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";

import Button from "button";
import LED from "led/digital";

class Backlight {
	#value = 1;

	set brightness(value) {
		if (value <= 0)
			value = 0;
		else if (value >= 1)
			value = 1;
		this.#value = value;
		if (globalThis.screen)
			globalThis.screen.brightness = value * 100;
	}
	get brightness() {
		return this.#value;
	}
	close() {}
}

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: Digital,
			pin: device.pin.buttonA,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonB {
	constructor(options) {
		return new Button({
			...options,
			io: Digital,
			pin: device.pin.buttonB,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 43,
			clock: 44
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 4
		}
	},
	io: { Analog, Digital, DigitalBank, I2C, PulseCount, PWM, Serial, SMBus, SPI },
	pin: {
		button: 0,
		buttonA: 0,
		buttonB: 21,
		led: 38,
		battery: 4
	},
	peripheral: {
		Backlight,
		led: {
			Default: class {
				constructor() {
					return new LED({
						io: Digital,
						pin: device.pin.led,
						mode: Digital.Output
					});
				}
			}
		},
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB
		},
	}
};

export default device;
