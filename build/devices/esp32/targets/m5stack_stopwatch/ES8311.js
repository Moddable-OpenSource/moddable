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

export default class ES8311 {
	#io;

	constructor(options) {
		this.#io = new options.io({hz: 100_000, address: 0x18, ...options});
	}

	configure({input = false, output = false}) {
		const io = this.#io;
		io.writeUint8(0x00, 0x80);
		io.writeUint8(0x01, input && output ? 0xBF : input ? 0xBA : 0xB5);
		io.writeUint8(0x02, 0x18);
		io.writeUint8(0x09, 0x0C);
		io.writeUint8(0x0A, 0x0C);
		io.writeUint8(0x0D, 0x01);
		if (input) {
			io.writeUint8(0x0E, 0x02);
			io.writeUint8(0x14, 0x17);
			io.writeUint8(0x17, 0xCB);
			io.writeUint8(0x1C, 0x64);
		}
		if (output) {
			io.writeUint8(0x12, 0x00);
			io.writeUint8(0x13, 0x10);
			io.writeUint8(0x32, 0xBF);
			io.writeUint8(0x37, 0x08);
		}
	}

	start() {
		// ADC must be armed after the I2S clocks have stabilized.
		for (let i = 0; i < 3; i++) {
			Timer.delay(30);
			this.#io.writeUint8(0x0D, 0x01);
		}
	}

	close() {
		const io = this.#io;
		if (!io)
			return;
		try {
			io.writeUint8(0x12, 0x02);
			io.writeUint8(0x0D, 0xFC);
			io.writeUint8(0x0E, 0x6A);
			io.writeUint8(0x00, 0x00);
		}
		finally {
			this.#io = undefined;
			io.close();
		}
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
