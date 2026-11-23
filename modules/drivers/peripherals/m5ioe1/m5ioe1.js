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

const PWMRegister = Object.freeze({8: 0x1E, 9: 0x1C, 10: 0x22, 11: 0x20});

export default class M5IOE1 {
	#io;

	constructor(options) {
		this.#io = new options.io({hz: 100_000, address: 0x4F, ...options});
		try {
			this.#io.writeUint8(0x23, 0);
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	configure({pin, output = true}) {
		const index = this.#index(pin);
		const bank = index >> 3;
		const mask = 1 << (index & 7);
		const pwm = PWMRegister[pin];
		if (pwm !== undefined)
			this.#update(pwm, 0x80, 0);
		this.#update(0x13 + bank, mask, 0);
		this.#update(0x03 + bank, mask, output ? mask : 0);
	}

	write(pin, value) {
		const index = this.#index(pin);
		const mask = 1 << (index & 7);
		this.#update(0x05 + (index >> 3), mask, value ? mask : 0);
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}

	#index(pin) {
		if (!Number.isInteger(pin) || pin < 1 || pin > 14)
			throw new RangeError("pin must be 1..14");
		return pin - 1;
	}
	#update(register, mask, value) {
		const io = this.#io;
		io.writeUint8(register, (io.readUint8(register) & ~mask) | value);
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
