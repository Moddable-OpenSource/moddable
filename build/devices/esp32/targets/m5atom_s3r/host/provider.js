/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

import Timer from "timer";
import IMU from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";

import Button from "button";

class Backlight {
	#io;

	constructor(options) {
		const io = this.#io = new device.io.SMBus({
			...device.I2C.internal,
			hz: 400_000,
			address: 48
		});

		io.writeUint8(0x00, 0b01000000);
		Timer.delay(1);
		io.writeUint8(0x08, 0b00000001);
		io.writeUint8(0x70, 0b00000000);
	}
	close() {
		this.#io?.close();
		this.#io = undefined;
	}
	set brightness(value) {
		if (value <= 0)
			value = 0;
		else if (value >= 1)
			value = 255;
		else
			value *= 255;
		this.#io.writeUint8(0x0e, value);
	}
}

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

class ButtonFlash {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonFlash,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 2,
			clock: 1
		},
		internal: {
			io: "I2C",
			data: 45,
			clock: 0
		}
	},
	SPI: {
		default: {
			io: "SPI",
			port: 3,
			clock: 15,
			out: 21
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 8
		}
	},
	pin: {
		button: 41,
		buttonA: 41,
		buttonFlash: 0,
		displaySelect: 14,
		IRTX: 47
	},
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight();
			}
		},
		button: {
			Default: ButtonFlash,
			A: ButtonA,
			Flash: ButtonFlash
		},
	},
	sensor: {
		IMU: class {
			constructor(options) {
				return new IMU({
					...options,
					sensor: {
						...device.I2C.internal,
						address: 0x69,
						io: device.io.SMBus
					}
				});
			}
		}
	}
};

export default device;
