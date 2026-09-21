/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

import RTC from "embedded:RTC/PCF8563"

import Timer from "timer";

import Button from "button";
import LED from "led/pwm";

const notes = new Map;
notes.set("C", 4186);
notes.set("Db", 4435);
notes.set("C#", 4435);
notes.set("D", 4699);
notes.set("D#", 4978);
notes.set("Eb", 4978);
notes.set("E", 5274);
notes.set("F", 5588);
notes.set("F#", 5920);
notes.set("Gb", 5920);
notes.set("G", 6272);
notes.set("G#", 6645);
notes.set("Ab", 6645);
notes.set("A", 7040);
notes.set("A#", 7459);
notes.set("Bb", 7459);
notes.set("B", 790);

class Tone {
	#io;
	#timer;
	
	constructor() {
		this.#io = new device.io.PWM({pin: device.pin.buzzer});
	}
	close() {
		this.#io?.close();
		if (this.#timer)
			Timer.clear(this.#timer);
		this.#io = this.#timer = undefined;
	}
	tone(hz, duration) {
		const io = this.#io = new device.io.PWM({from: this.#io, hz});
		io.write(512);
	
		if (duration) {
			if (this.#timer)
				Timer.schedule(this.#timer, duration);
			else
				this.#timer = Timer.set(() => {
					this.#timer = undefined;
					this.mute();
				}, duration);
		}
		else if (this.#timer) {
			Timer.clear(this.#timer);
			this.#timer = undefined;
		}
	}
	note(note, octave = 4, duration) {
		note = notes.get(note);
		if (!note || (octave > 8))
			throw new Error;
		this.tone(Math.idiv(note, (1 << (8 - octave))), duration);
	}
	mute() {
		this.#io.write(0);
	}
}

class ButtonB {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 38,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonUp {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 37,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonDown {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 39,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 5,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonPower {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 27,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 21,
			clock: 22
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 18,
			out: 23,
			port: 2		// VSPI_HOST
		}
	},
	pin: {
		button: 5,
		buttonA: 5,
		buttonB: 38,
		led: 10,
		buzzer: 2,
		powerMain: 12,
		epdSelect: 9, 
		epdDC: 15, 
		epdReset: 0,
		epdBusy: 4,
		rtcInterrupt: 19,
		batteryVoltage: 35
	},
	sensor: {
	},
	peripheral: {
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.i2c.default,
						io: device.io.SMBus
					},
					interrupt: {
						io: device.io.Digital,
						pin: device.pin.rtcInterrupt
					}
				});
			}
		},
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			Up: ButtonUp,
			Down: ButtonDown,
			Middle: ButtonB,
			External: ButtonA,
			Power: ButtonPower
		},
		led: {
			Default: class {
				constructor() {
					return new LED({
						io: device.io.PWM,
						pin: device.pin.led,
						invert: true
					});
				}
			}
		},
		tone: {
			Default: Tone
		},
		battery: {
			Default: class {
				#analog;
				constructor() {
					this.#analog = new device.io.Analog({pin: device.pin.batteryVoltage});
				}
				close() {
					this.#analog?.close();
				}
				read() {
					let value = this.#analog.read() * 3300;
					value *= 25.1 / 5.1 / 1000;
					return value / (1 << this.#analog.resolution);
				}
			}
		}
	}
};

export default device;
