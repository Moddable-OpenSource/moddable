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

// Allocate shared state at runtime, outside the frozen preloaded module.
const expanderState = Symbol("NessoN1Expander");

// Board peripherals currently use the eight pins on port 1.
const DIRECTION = 0x03;
const OUTPUT = 0x05;
const HIGH_IMPEDANCE = 0x07;
const PULL_ENABLE = 0x0B;
const PULL_SELECT = 0x0D;
const INPUT = 0x0F;

class DigitalBank {
	static Input = 0;
	static InputPullUp = 1;
	static Output = 8;

	#expander;
	#pins;
	#mode;

	constructor({pins, mode, initial = 0}, expander) {
		if (!Number.isInteger(pins) || pins < 1 || pins > 0xFF)
			throw new RangeError("invalid pins");
		this.#expander = expander;
		this.#pins = pins;
		this.#mode = mode;

		switch (mode) {
			case DigitalBank.Output:
				this.#update(OUTPUT, initial);
				this.#update(DIRECTION, pins);
				this.#update(HIGH_IMPEDANCE, 0);
				break;
			case DigitalBank.InputPullUp:
				this.#update(DIRECTION, 0);
				this.#update(PULL_SELECT, pins);
				this.#update(PULL_ENABLE, pins);
				break;
			case DigitalBank.Input:
				this.#update(DIRECTION, 0);
				this.#update(PULL_ENABLE, 0);
				break;
			default:
				throw new RangeError("invalid mode");
		}
	}

	#update(register, value) {
		const io = this.#expander;
		io.writeUint8(register, (io.readUint8(register) & ~this.#pins) | (value & this.#pins));
	}

	read() {
		if (this.#mode === DigitalBank.Output)
			throw new Error("output pins");
		return this.#expander.readUint8(INPUT) & this.#pins;
	}

	write(value) {
		if (this.#mode !== DigitalBank.Output)
			throw new Error("input pins");
		this.#update(OUTPUT, value);
	}

	close() {
		this.#expander = undefined;
	}

	[Symbol.dispose]() {
		this.close();
	}
}

class Digital extends DigitalBank {
	constructor({pin, initial = 0, ...options}, expander) {
		if (!Number.isInteger(pin) || pin < 0 || pin > 7)
			throw new RangeError("invalid pin");
		super({...options, pins: 1 << pin, initial: initial ? 0xFF : 0}, expander);
	}

	read() {
		return super.read() ? 1 : 0;
	}

	write(value) {
		super.write(value ? 0xFF : 0);
	}
}

export default class Expander {
	#entry;

	constructor(index) {
		const expanders = globalThis[expanderState] ??= [
			{address: 0x43, users: 0, io: undefined},
			{address: 0x44, users: 0, io: undefined}
		];
		const entry = expanders[index];
		entry.io ??= new device.io.SMBus({...device.i2c.internal, address: entry.address, hz: 100_000});
		entry.users++;
		this.#entry = entry;
		const expander = this;
		this.Digital = class extends Digital {
			constructor(options) {
				super(options, expander);
			}
		};
		this.DigitalBank = class extends DigitalBank {
			constructor(options) {
				super(options, expander);
			}
		};
	}

	readUint8(register) {
		return this.#entry.io.readUint8(register);
	}

	writeUint8(register, value) {
		this.#entry.io.writeUint8(register, value);
	}

	close() {
		const entry = this.#entry;
		if (!entry)
			return;
		this.#entry = undefined;
		if (--entry.users === 0) {
			const io = entry.io;
			entry.io = undefined;
			io.close();
		}
	}
}
