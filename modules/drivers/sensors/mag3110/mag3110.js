/*
 * Copyright (c) 2019-2026 Moddable Tech, Inc.
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
/*
	NXP MAG3110 magnetometer
	https://www.nxp.com/docs/en/data-sheet/MAG3110.pdf

	sample() values are microtesla
*/

import Timer from "timer";

const Register = Object.freeze({
	OUT_X: 0x01,
	WHO_AM_I: 0x07,
	CTRL_REG1: 0x10,
	CTRL_REG2: 0x11
});

const EXPECTED_WHO_AM_I = 0xC4;
const MICROTESLA_PER_LSB = 0.1;

class MAG3110 {
	#io;
	#raw = new ArrayBuffer(6);
	#view;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x0E,
			...options.sensor
		});
		this.#view = new DataView(this.#raw);

		try {
			if (io.readUint8(Register.WHO_AM_I) !== EXPECTED_WHO_AM_I)
				throw new Error("unexpected sensor");

			io.writeUint8(Register.CTRL_REG1, 0);
			io.writeUint8(Register.CTRL_REG2, 0b1011_0000);
			Timer.delay(100);
			io.writeUint8(Register.CTRL_REG1, 0b0000_0001);
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	configure(options) {
		const io = this.#io;
		if ("active" in options)
			io.writeUint8(Register.CTRL_REG1, options.active ? 0b0000_0001 : 0);
	}
	close() {
		this.#io?.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		io.readBuffer(Register.OUT_X, this.#raw);
		const view = this.#view;
		return {
			x: view.getInt16(0) * MICROTESLA_PER_LSB,
			y: view.getInt16(2) * MICROTESLA_PER_LSB,
			z: view.getInt16(4) * MICROTESLA_PER_LSB
		};
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default MAG3110;
