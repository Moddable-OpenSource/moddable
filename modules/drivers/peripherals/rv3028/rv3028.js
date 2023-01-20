/*
 * Copyright (c) 2021-2023  Moddable Tech, Inc.
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
	RV3028 - Micro Crystal Extreme Low Power Real-Time Clock Module
	https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf
*/

const Register = Object.freeze({
	UNIXTIME:		0x1B,
	STATUS:			0x0e,
	POWER_ON_RESET:	0x01,
	ENABLE_BIT:		0x01
});

class RV3028 {
	#io;
	#blockBuffer = new Uint8Array(4);

	constructor(options) {
		const { clock } = options;
		const io = this.#io = new clock.io({
			hz: 400_000,
			address: 0x52,
			...clock
		});

		try {
			io.readUint8(0);
		}
		catch (e) {
			io.close();
			throw e;
		}
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	configure(options) {
	}
	get configuration() {
		return {};
	}
	get time() {
		const reg = this.#blockBuffer;

		if (this.#io.readUint8(Register.STATUS) & Register.POWER_ON_RESET)
			return undefined;

		this.#io.readBuffer(Register.UNIXTIME, reg);

		// UNIXTIME
		return new Date( ((reg[3] << 24) | (reg[2] << 16) | (reg[1] << 8) | reg[0]) * 1000 );
	}

	set time(v) {
		let io = this.#io;
		let b = this.#blockBuffer;
		v /= 1000;

		b[0] = v & 0xff;
		b[1] = (v & 0xff00) >> 8;
		b[2] = (v & 0xff0000) >> 16;
		b[3] = (v & 0xff000000) >> 24;

		io.writeBuffer(Register.UNIXTIME, b);

		io.writeUint8(Register.STATUS, 0);		// enable
	}
}

export default RV3028;
