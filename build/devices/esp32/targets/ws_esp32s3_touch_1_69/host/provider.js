/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/Touch/CST816";
import PCF85063 from "embedded:RTC/PCF85063";
import IMU from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";

import Button from "button";
import Backlight from "backlight";

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
			data: 11,
			clock: 10
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 44,
			transmit: 43
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 6,
			in: -1,
			out: 7,
			port: 2
		}
	},
	pin: {
		//@@ button
		button: 0,
		buttonA: 0,
		buttonB: 40,
		batteryADC: 1,
		touchReset: 13,
		touchInterrupt: 14,
		backlight: 15,
		imuInterrupt: 38,
		rtcInterrupt: 39,
		buzzer: 42
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			Flash: ButtonA
		},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
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
						pin: device.pin.touchReset
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: device.pin.touchInterrupt
					}
				});
				result.configure({});
				return result;
			}
		},
		IMU: class {
			constructor(options) {
				return new IMU({
					...options,
					sensor: {
						...device.i2c.default,
						address: 0x6b,
						io: device.io.SMBus
					},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
						pin: device.pin.imuInterrupt
					}
				});
			}
		}
	}
};

device.rtc = {
	io: PCF85063,
	clock: {
		...device.i2c.default,
		io: "SMBus"
	},
	interrupt: {
		io: "Digital",
		pin: device.pin.rtcInterrupt
	}
};

export default device;
