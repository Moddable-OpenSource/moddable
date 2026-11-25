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

import LED from "led/apa102";
import Backlight from "backlight";
import Button from "button";
import Touch from "embedded:sensor/Touch/CST816";

class ButtonFlash {
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
		default: {io: "I2C", data: 11, clock: 14},
		internal: {io: "I2C", data: 11, clock: 14}
	},
	pin: {
		button: 0,
		backlight: 18,
		amplifierEnable: 45,
		ledData: 38,
		ledClock: 39
	},
	sensor: {
		Touch: class {
			constructor(options = {}) {
				return new Touch({
					...options,
					sensor: {...device.i2c.internal, io: device.io.SMBus},
					interrupt: {io: device.io.Digital, mode: device.io.Digital.Input, pin: 12}
				});
			}
		}
	},
	peripheral: {
		led: {
			Default: class {
				constructor(options = {}) {
					return new LED({
						brightness: 32,
						...options,
						io: device.io.Digital,
						data: device.pin.ledData,
						clock: device.pin.ledClock
					});
				}
			}
		},
		button: {Default: ButtonFlash, Flash: ButtonFlash},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight,
					hz: 2000,
					resolution: 8,
					invert: true
				});
			}
		}
	}
};

export default device;
