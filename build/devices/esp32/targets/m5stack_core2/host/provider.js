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

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import PulseWidth from "embedded:io/pulsewidth";
import RTC from "embedded:RTC/BM8563";
import Touch from "M5StackCoreTouch";
import Core2Power from "Core2Power";
import Core2IMU from "Core2IMU";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 32,
			clock: 33
		},
		internal: {
			io: I2C,
			data: 21,
			clock: 22
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	SPI: {
		default: {
			io: SPI,
			clock: 18,
			in: 38,
			out: 23,
			port: 1
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 35
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PulseCount, PulseWidth, PWM, Serial, SMBus, SPI},
	pin: {
		displayDC: 15,
		displaySelect: 5
	},
	sensor: {
		Touch: class {
			constructor(options) {
				const result = new Touch({
					...options,
					sensor: {
						...device.I2C.internal,
						io: device.io.SMBus,
              		},
					interrupt: {
						io: Digital,
						mode: Digital.Input,
						pin: 39
					}
				});
            	result.configure({threshold: 20});
            	return result;
			}
		},
		IMU: class {
			constructor(options) {
				return new Core2IMU({
					...options,
					sensor: {
						...device.I2C.internal,
						io: device.io.SMBus
					}
				});
			}
		}
	},
	peripheral: {
		Power: class {
			constructor(options) {
				return new Core2Power({
					...options,
					peripheral: {
						...device.I2C.internal,
						io: device.io.SMBus
					}
				});
			}
		},
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.I2C.internal,
						io: SMBus,
					},
				});
			}
		},
	}
};

export default device;
