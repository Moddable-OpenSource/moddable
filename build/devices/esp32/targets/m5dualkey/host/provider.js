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

import Button from "button";
import LEDneopixel from "led/neopixel";
import NeoPixel from "neopixel";

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
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 47,
			transmit: 48
		},
		chain1: {
			io: "Serial",
			port: 1,
			receive: 47,
			transmit: 48
		},
		chain2: {
			io: "Serial",
			port: 2,
			receive: 5,
			transmit: 6
		}
	},
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
			A: ButtonA,
			B: ButtonB,
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
							io: device.io.Digital,
							pin: device.pin.ledPower
						}
					});
				}
			}
		},
		battery: {
			Default: class {
				#analog;
				constructor(options) {
					if ("target" in options)
						this.target = options.target;
					try {
						this.#analog = new device.io.Analog({
							pin: device.pin.battery,
							esp32: {
								calibrated: false,
								attenuation: 12
							}
						});
					}
					catch (e) {
						this.close();
						throw e;
					}
				}
				close() {
					this.#analog?.close();
					this.#analog = undefined;
				}
				read() {
					const raw = this.#analog.read();
					return raw * 3333 * 1.51 / 1000
						/ ((1 << this.#analog.resolution) - 1);
				}
				static {
					this.prototype[Symbol.dispose] = this.prototype.close;
				}
			}
		}
	}
};

export default device;
