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

import Button from "button";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: Digital,
			pin: device.pin.buttonA,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonB {
	constructor(options) {
		return new Button({
			...options,
			io: Digital,
			pin: device.pin.buttonB,
			mode: Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 47,
			transmit: 48
		},
		chain1: {
			io: Serial,
			port: 1,
			receive: 47,
			transmit: 48
		},
		chain2: {
			io: Serial,
			port: 2,
			receive: 5,
			transmit: 6
		}
	},
	io: { Analog, Digital, DigitalBank, I2C, PulseCount, PWM, Serial, SMBus, SPI },
	pin: {
		buttonA: 0,
		buttonB: 17,
		led: 21,
		ledPower: 40,
		switch1: 8,
		switch2: 7,
		battery: 10,
		vbus: 2,
		charge: 9
	},
	peripheral: {
		button: {
			Default: ButtonA,
			buttonA: ButtonA,
			buttonB: ButtonB
		},
		led: {
			Default: class {
				constructor(options) {
					return new LEDneopixel({
						...options,
						neopixels: {
							io: NeoPixel,
							length: 2,
							pin: device.pin.led,
							order: "GRB",
							brightness: 32
						},
						power: {
							io: Digital,
							pin: device.pin.ledPower
						}
					});
				}
			}
		},
		battery: {
			Default: class {
				#analog;
				constructor() {
					this.#analog = new Analog({
						pin: device.pin.battery,
						esp32: {
							calibrated: false,
							attenuation: 12
						}
					});
				}
				close() {
					this.#analog?.close();
				}
				read() {
					const raw = this.#analog.read();
					return raw * 3333 * 1.51 / 1000
						/ ((1 << this.#analog.resolution) - 1);
				}
			}
		}
	}
};

export default device;
