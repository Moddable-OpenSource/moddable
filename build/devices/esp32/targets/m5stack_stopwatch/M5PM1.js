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

import Timer from "timer";

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
			// A sleeping M5PM1 wakes on the first I²C transaction and does not acknowledge it.
			this.#io.writeQuick();
			Timer.delay(10);
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	configure() {
		const io = this.#io;
		io.writeUint8(Register.I2CSleep, 0);
		io.writeUint8(Register.Watchdog, 0);
		io.writeUint8(Register.PowerConfig, io.readUint8(Register.PowerConfig) | 0x17);
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
