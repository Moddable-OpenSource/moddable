/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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
// import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";

const device = {
	I2C: {
		default: {
			io: I2C,
			port: "I2C_NUM_0",
			data: 8,
			clock: 9
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 0,
			receive: 20,
			transmit: 21
		}
	},
	SPI: {
		default: {
			io: SPI,
			port: 1,
			clock: 6,
			in: 2,
			out: 7
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 1
		}
	},
	io: { Analog, Digital, DigitalBank, I2C, PWM, Serial, SMBus, SPI },
	pin: {
		button: 0,
		led: 19
	}
};

export default device;
