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

import IMU from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";

import Button from "button";

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
		default: {
			io: "I2C",
			data: 45,
			clock: 0
		},
		internal: {
			io: "I2C",
			data: 45,
			clock: 0
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
			in: 37,
			out: 35,
			port: 2
		}
	},
	pin: {
		button: 0,
		buttonA: 0,
		power: 18,
		IRTX: 47
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
		},
		Power: {
			Camera: class {
				constructor() {
					return new device.io.Digital({
						io: device.io.Digital,
						pin: device.pin.power,
						mode: device.io.Digital.Output
					});
				}
			}
		}
	},
	sensor: {
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

export default device;
