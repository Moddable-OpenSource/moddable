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
import CST9217 from "embedded:sensor/Touch/CST9217";
import RTC from "embedded:RTC/PCF8563";

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
			io: device.io.Digital,
			pin: device.pin.buttonA,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {io: "I2C", data: 7, clock: 6}
	},
	pin: {
		button: 0,
		buttonA: 0,
		sdSelect: 38,
		touchInterrupt: 9,
		displayEnable: 16
	},
	peripheral: {
		Backlight,
		button: {
			Default: ButtonA,
			A: ButtonA
		}
	},
	sensor: {
		Touch: class {
			constructor(options) {
				const touch = new CST9217({
					...options,
					sensor: {
						...device.i2c.default,
						address: 0x5A,
						io: device.io.I2C
					},
					interrupt: {
						io: device.io.Digital,
						pin: device.pin.touchInterrupt,
						mode: device.io.Digital.InputPullUp
					}
				});
				touch.configure({length: 1, active: true, flip: "hv", width: 466, height: 466});
				return touch;
			}
		}
	}
};

device.rtc = {
	io: RTC,
	clock: {...device.i2c.default, io: "SMBus"}
};

export default device;
