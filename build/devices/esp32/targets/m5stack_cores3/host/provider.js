/*
 * Copyright (c) 2023 Shinya Ishikawa
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
import RTC from "embedded:RTC/BM8563"

const device = {
  I2C: {
    default: {
      io: I2C,
      data: 2,
      clock: 1,
    },
    internal: {
      io: I2C,
      data: 12,
      clock: 11,
    },
  },
  Serial: {
    default: {
      io: Serial,
      port: 1,
      receive: 44,
      transmit: 43,
    },
  },
  SPI: {
    default: {
      io: SPI,
      clock: 36,
      in: 35,
      out: 37,
      port: 1,
    },
  },
  Analog: {
    default: {
      io: Analog,
      pin: 10,
    },
  },
  io: {
    Analog,
    Digital,
    DigitalBank,
    I2C,
    PulseCount,
    PulseWidth,
    PWM,
    Serial,
    SMBus,
    SPI,
  },
  pin: {
    displayDC: 15,
    displaySelect: 5,
  },
  peripheral: {
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.I2C.internal,
						io: SMBus
					}
				});
			}
		}
  }
};

export default device;
