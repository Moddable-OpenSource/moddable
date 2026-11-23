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

const Register = Object.freeze({
	PowerSource: 0x04,
	WakeSource: 0x05,
	PowerConfig: 0x06,
	I2CSleep: 0x09,
	Watchdog: 0x0A,
	GPIOInput: 0x12
});

export default class M5PM1 {
	#io;

	constructor(options) {
		this.#io = new options.io({hz: 100_000, address: 0x6E, ...options});
		try {
			this.#io.writeUint8(Register.I2CSleep, 0);
			this.#io.writeUint8(Register.Watchdog, 0);
			this.#io.writeUint8(Register.PowerConfig, this.#io.readUint8(Register.PowerConfig) | 0x17);
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	get status() {
		const io = this.#io;
		return {
			powerSource: io.readUint8(Register.PowerSource) & 7,
			wakeSource: io.readUint8(Register.WakeSource) & 0x7F,
			inputs: io.readUint8(Register.GPIOInput) & 0x1F
		};
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
