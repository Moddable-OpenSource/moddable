/*
 * Copyright (c) 2023-2026 Shinya Ishikawa
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

import RTC from "embedded:RTC/BM8563"
import Touch from "M5StackCoreS3Touch";
import IMU from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";


const device = {
  i2c: {
    default: {
      io: "I2C",
      data: 2,
      clock: 1
    },
    internal: {
      io: "I2C",
      data: 12,
      clock: 11
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
      clock: 36,
      in: 35,
      out: 37,
      port: 1
    }
  },
  analog: {
    default: {
      io: "Analog",
      pin: 10
    }
  },
  pin: {
    displayDC: 15,
    displaySelect: 5
  },
  peripheral: {
		// Power.brightness is percent 0–100 (AXP2101 DLDO1)
		Backlight: class {
			#value = 1;
			set brightness(value) {
				if (value <= 0)
					value = 0;
				else if (value >= 1)
					value = 1;
				this.#value = value;
				if (undefined !== globalThis.power)
					globalThis.power.brightness = value * 100;
			}
			get brightness() {
				return this.#value;
			}
			close() {
			}
		},
  },
  sensor :{
        Touch: class {
          constructor(options) {				
            const result = new Touch({
              ...options,
              sensor: {
                ...device.i2c.internal,
                io: device.io.SMBus,
              }
            });
            result.configure({threshold: 20});
            return result;
          }
        },
		IMU: class {
			constructor(options) {
				return new IMU({
					...options,
					sensor: {
						...device.i2c.internal,
						address: 0x69,
						io: device.io.SMBus
					}
				});
			}
		}
  }
};

device.rtc = {
	io: RTC,
	clock: {
		...device.i2c.internal,
		io: "SMBus"
	}
};

export default device;
