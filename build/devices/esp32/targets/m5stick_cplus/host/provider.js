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

import AXP192 from "embedded:peripheral/Power/axp192";
import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";
import RTC from "embedded:RTC/BM8563";
import Button from "button";
import LED from "led/pwm";

/**
 * LCD backlight via AXP192 LDO2 (Power.brightness 0–100).
 * brightness 0–1 matches shared PWM Backlight / piu setup path.
 */
class Backlight {
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
			data: 32,
			clock: 33
		},
		internal: {
			io: "I2C",
			data: 21,
			clock: 22
		},
		hat: {
			io: "I2C",
			data: 0,
			clock: 26
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
			clock: 13,
			out: 15,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 36
		}
	},
	pin: {
		button: 37,
		buttonA: 37,
		buttonB: 39,
		led: 10,
		displayDC: 23,
		displaySelect: 5
	},
	sensor: {
		IMU: class extends MPU6886 {
			constructor(options) {
				super({
					...options,
					sensor: {
						...device.i2c.internal,
						io: device.io.SMBus
					}
				});
			}
			sample() {
				const sample = super.sample();
				sample.accelerometer.x *= -1;
				sample.accelerometer.y *= -1;
				sample.gyroscope.y *= -1;
				return sample;
			}
		}
	},
	peripheral: {
		Backlight,
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.PWM,
						pin: device.pin.led,
						invert: true		// active-low red LED
					});
				}
			}
		},
		Power: class extends AXP192 {
			#brightness = 100;

			constructor(options) {
				super({
					...options,
					peripheral: {
						...device.i2c.internal,
						io: device.io.SMBus
					}
				});
				this.writeUint8(0x10, 0xff); // OLED VPP Enable
				this.writeUint8(0x28, 0xff); // Enable LDO2&LDO3, LED&TFT 3.3V
				this.writeUint8(0x82, 0xff); // Enable all the ADCs
				this.writeUint8(0x33, 0xc0); // Enable Charging, 100mA, 4.2V End at 0.9
				this.writeUint8(0xb8, 0x80); // Enable Colume Counter
				this.writeUint8(0x12, 0x4d); // Enable DC-DC1, OLED VDD, 5B V EXT
				this.writeUint8(0x36, 0x5c); // PEK
				this.writeUint8(0x91, 0xA0); // Set MIC voltage to 2.8V
				this.writeUint8(0x90, 0x02); // gpio0
				this.brightness = 100;
			}

			/**
			 * LCD backlight level, 0–100.
			 * Hardware: AXP192 LDO2 voltage nibble (reg 0x28 bits 7–4), codes 7–15.
			 */
			set brightness(value) {
				if (value <= 0)
					value = 0;
				else if (value >= 100)
					value = 100;
				this.#brightness = value;

				let code;
				if (value <= 0)
					code = 7;
				else if (value >= 100)
					code = 15;
				else
					code = (value / 100) * 8 + 7;
				this.writeUint8(0x28, (this.readUint8(0x28) & 0x0F) | (code << 4));
			}
			get brightness() {
				return this.#brightness;
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
