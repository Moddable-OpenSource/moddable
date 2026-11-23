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
import CST820 from "embedded:sensor/Touch/CST820";
import CST9217 from "embedded:sensor/Touch/CST9217";
import audio from "m5stack_stopwatch/audio";

class Backlight {
	#brightness = 1;

	get brightness() {
		return this.#brightness;
	}
	set brightness(value) {
		this.#brightness = Math.max(0, Math.min(1, value));
		if (globalThis.screen)
			screen.brightness = this.#brightness * 100;
	}
	write(value) {
		this.brightness = value / 100;
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
			mode: device.io.Digital.Input,
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
			mode: device.io.Digital.Input,
			activeLow: true
		});
	}
}

class Touch {
	#touch;
	#scale;

	constructor(options = {}) {
		if ("target" in options)
			this.target = options.target;

		this.#scale = options.coordinateScale ?? 1;
		if (this.#scale !== 1 && this.#scale !== 2)
			throw new RangeError("coordinateScale must be 1 or 2");

		const bus = device.i2c.internal;
		const probe = new device.io.I2C({...bus, address: 0x5A, hz: 100_000});
		let cst9217;
		try {
			cst9217 = !probe.write(new Uint8Array(0));
		}
		finally {
			probe.close();
		}

		const Driver = cst9217 ? CST9217 : CST820;
		const onSample = options.onSample;
		this.#touch = new Driver({
			...options,
			sensor: {...bus, io: cst9217 ? device.io.I2C : device.io.SMBus},
			interrupt: {io: device.io.Digital, pin: device.pin.touchInterrupt, mode: device.io.Digital.Input},
			onSample: onSample ? () => onSample.call(this) : undefined
		});
		if (cst9217)
			this.#touch.configure({length: 1, width: 466, height: 466});
	}

	configure(options) {
		this.#touch.configure(options);
	}
	get configuration() {
		return this.#touch.configuration;
	}
	get identification() {
		return this.#touch.identification;
	}
	sample() {
		const points = this.#touch.sample();
		if (points && this.#scale === 2) {
			for (const point of points) {
				point.x = Math.min(465, point.x * 2);
				point.y = Math.min(465, point.y * 2);
			}
		}
		return points;
	}
	close() {
		this.#touch?.close();
		this.#touch = undefined;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

const externalI2C = {io: "I2C", data: 10, clock: 11};
const device = {
	audio,
	i2c: {
		default: externalI2C,
		external: externalI2C,
		internal: {io: "I2C", data: 47, clock: 48}
	},
	pin: {button: 2, buttonA: 2, buttonB: 1, touchInterrupt: 13, displayTE: 38},
	peripheral: {button: {Default: ButtonA, A: ButtonA, B: ButtonB}, Backlight},
	sensor: {Touch}
};

export default device;
