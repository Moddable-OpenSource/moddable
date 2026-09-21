/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import Button from "button";
import LED from "led/digital";

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: Digital,
			pin: device.pin.button,
			mode: Digital.InputPullUp,
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
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonC {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonC,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonD {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonD,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: I2C,
			data: 26,
			clock: 27
		}
	},
	serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 30,
			transmit: 31
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PWM, Serial, SMBus},
	pin: {
		button: 11,
		buttonA: 11,
		buttonB: 12,
		buttonC: 24,
		buttonD: 25,
		led: 13,
		ledA: 13,
		ledB: 14,
		ledC: 15,
		ledD: 16
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			C: ButtonC,
			D: ButtonD
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: Digital,
						pin: device.pin.led,
						invert: false
					});
				}
			}
		}
	}
};

export default device;
