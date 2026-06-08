/*
 * Copyright (c) 2021-2024  Moddable Tech, Inc.
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
import Touch from "embedded:sensor/Touch/GT911";
import PulseWidth from "embedded:io/pulsewidth";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 4,
			clock: 5
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
			clock: 15,
			in: 16,
			out: 17,
			port: 1
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PulseCount, PulseWidth, PWM, Serial, SMBus, SPI},
	pin: {
		//@@ button
		button: 0,
	},
	sensor: {
		Touch: class {
			constructor(options) {				
				const address = [0x14, 0x5D].find(address => {
					let result = 1, i;
					try {
						i = new I2C({
							...device.I2C.default,
							hz: 200_000,
							address
						});
						result = i.write(new ArrayBuffer);		// SMBus write quick (see linux/drivers/i2c/i2c-core-base.c => i2c_default_probe)
					}
					catch {
					}
					i?.close();
					return undefined === result;
				});

				const result = new Touch({
					...options,
					sensor: {
						...device.I2C.default,
						io: device.io.I2C,
						address
					},
					interrupt: {
						io: Digital,
						mode: Digital.Input,
						pin: 38
					}
				});
				result.configure({threshold: 20});
				return result;
			}
		}
	}
};

export default device;
