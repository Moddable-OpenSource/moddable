/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
	MLX90614 sensor - data sheet at https://www.melexis.com/en/documents/documentation/datasheets/datasheet-mlx90614
	or https://components101.com/asset/sites/default/files/component_datasheet/MLX90614-Datasheet.pdf

*/

import CRC8 from "crc";

const Register = {
	MLX90614_TA:		0x06,
	MLX90614_TOBJ1:		0x07,
};
Object.freeze(Register);

class MLX90614 {
	#io;
	#byteBuffer = new Uint8Array(1);
	#valueBuffer = new Uint8Array(3);
	#pecBuffer = new Uint8Array(5);
	#address;	// for PEC
	#crc;
	#onError;

	constructor(options) {
		this.#address = options.sensor.address ?? 0x5A;
		const io = this.#io = new options.sensor.io({
			hz: 100_000,
			address: this.#address,
			...options.sensor
		});

		this.#onError = options.onError;

		this.#crc = new CRC8(0x07);
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		let value = {};
		value.ambientTemperature = this.#readTemp(Register.MLX90614_TA);
		value.temperature = this.#readTemp(Register.MLX90614_TOBJ1);
		return value;
	}
	#readTemp(reg) {
		const io = this.#io;
		const vBuf = this.#valueBuffer;
		const pBuf = this.#pecBuffer;
		io.readBuffer(reg, vBuf);

		pBuf[0] = this.#address << 1;
		pBuf[1] = reg;
		pBuf[2] = (this.#address << 1) + 1;
		pBuf[3] = vBuf[0];
		pBuf[4] = vBuf[1];

		if (this.#crc.reset().checksum(pBuf) !== vBuf[2])
			this.onError?.("bad checksum");

		let value = (vBuf[1] << 8) | vBuf[0];
		return (value * 0.02) - 273.15;
	}
}
export default MLX90614;
