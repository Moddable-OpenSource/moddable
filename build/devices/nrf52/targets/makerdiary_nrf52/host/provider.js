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
import LEDrgb from "led/rgb";

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
		button: 18,
		buttonA: 18,
		led: 23,
		ledR: 23,
		ledG: 22,
		ledB: 24
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		led: {
			Default: class {
				constructor(options) {
					return new LEDrgb({
						...options,
						io: PWM,
						pin: { r: device.pin.ledR, g: device.pin.ledG, b: device.pin.ledB },
						invert: false
					});
				}
			}
		}
	}
};

export default device;
