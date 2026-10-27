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

import Backlight from "backlight";
import Button from "button";
import Touch from "embedded:sensor/Touch/FT6x36";

class ButtonBoot {
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
			data: 7,
			clock: 8
		},
		// OV5647 SCCB on the default bus. Image In opens this from camera.js.
		camera: {
			address: 0x36,
			hz: 400_000
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 21,
			out: 20,
			port: 1		// SPI2_HOST, the ST7796 bus
		}
	},
	pin: {
		button: 35,		// BOOT. PWR is the AXP2101 power key, not a GPIO.
		amplifier: 53,	// NS4150B enable, active high
		backlight: 28,
		displayDC: 26,
		displaySelect: 23
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
			Default: ButtonBoot,
			A: ButtonBoot
		}
	},
	sensor: {
		Touch: class {
			constructor(options) {
				const result = new Touch({
					...options,
					sensor: {
						...device.i2c.default,
						io: device.io.SMBus
					},
					reset: {
						io: device.io.Digital,
						mode: device.io.Digital.Output,
						pin: 29
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: 50
					}
				});
				result.configure({threshold: 32});
				return result;
			}
		}
	}
};

export default device;
