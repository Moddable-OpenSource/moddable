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

import RTC from "embedded:RTC/BM8563";
import Touch from "M5StackCoreTouch";
import Core2Power from "Core2Power";
import Core2IMU from "Core2IMU";

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 32,
			clock: 33
		},
		internal: {
			io: "I2C",
			data: 21,
			clock: 22
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 18,
			in: 38,
			out: 23,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 35
		}
	},
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
						...device.i2c.internal,
						io: device.io.SMBus,
              		},
					interrupt: {
						io: device.io.Digital,
						mode: device.io.Digital.Input,
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
						...device.i2c.internal,
						io: device.io.SMBus
					}
				});
			}
		}
	},
	peripheral: {
		// Core2Power.brightness is percent 0–100
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
			close() {}
		},
		Power: class {
			constructor(options) {
				return new Core2Power({
					...options,
					peripheral: {
						...device.i2c.internal,
						io: device.io.SMBus
					}
				});
			}
		},
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
