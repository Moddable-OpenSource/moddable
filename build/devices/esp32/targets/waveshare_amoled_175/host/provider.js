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
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import Touch from "embedded:sensor/Touch/CST9217";
import RTC from "embedded:RTC/PCF85063";
import QMI8658 from "embedded:sensor/Accelerometer-Gyroscope/QMI8658";

import Button from "button";
import AXP2101 from "embedded:peripheral/Power/axp2101";

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
	write(value) {
		this.brightness = value / 100;
	}
	close() {}
}

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonA,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 15,
			clock: 14
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 44,
			transmit: 43
		}
	},
	SPI: {
		default: {
			io: SPI,
			port: 2,
			clock: 38,
			out: 4
		}
	},
	io: { Analog, Digital, DigitalBank, I2C, PulseCount, PWM, Serial, SMBus, SPI },
	pin: {
		button: 0,
		buttonA: 0,
		amplifier: 46,
		touchReset: 40,
		touchInterrupt: 11,
		imuInterrupt: 21,
		displayTE: 13
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA
		},
		Backlight,
		Power: {
			PMIC: class {
				constructor() {
					return new AXP2101({
						sensor: { ...device.I2C.default, io: SMBus },
						address: 0x34
					});
				}
			},
			Amplifier: class {
				constructor() {
					return new Digital({
						pin: device.pin.amplifier,
						mode: Digital.Output,
						initialValue: 1
					});
				}
			}
		},
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.I2C.default,
						io: SMBus
					}
				});
			}
		}
	},
	sensor: {
		Touch: class {
			constructor(options) {
				const result = new Touch({
					...options,
					sensor: {
						...device.I2C.default,
						io: I2C,
						address: 0x5A
					},
					reset: {
						io: Digital,
						mode: Digital.Output,
						pin: device.pin.touchReset
					},
					interrupt: {
						io: Digital,
						mode: Digital.Input,
						pin: device.pin.touchInterrupt
					}
				});
				result.configure({ length: 2, flip: "hv" });
				return result;
			}
		},
		IMU: class extends QMI8658 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.I2C.default,
						io: SMBus,
						address: 0x6B
					},
					interrupt: {
						io: Digital,
						mode: Digital.Input,
						pin: device.pin.imuInterrupt
					}
				});
			}
			sample() {
				const sample = super.sample();
				sample.accelerometer.x *= -1;
				sample.gyroscope.x *= -1;
				return sample;
			}
		}
	}
};

export default device;
