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

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";

import Button from "button";
import LED from "led/pwm";

class ButtonFlash {
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
	I2C: {
		default: {
			io: I2C,
			data: 22,
			clock: 23
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 0,
			receive: 17,
			transmit: 16
		}
	},
	SPI: {
		default: {
			io: SPI,
			port: 2,
			clock: 19,
			in: 20,
			out: 18
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 0
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PWM, Serial, SMBus, SPI},
	pin: {
		button: 9,
		buttonA: 9,
		led: 15,
		rfSwitchEnable: 3,
		rfSwitchSelect: 14
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: PWM,
						pin: device.pin.led,
						invert: true
					});
				}
			}
		}
	}
};

export default device;
