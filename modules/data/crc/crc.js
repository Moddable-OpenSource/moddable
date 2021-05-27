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


class CRC8 {
	#poly;
	#initial;
	#reflectInput;
	#reflectOutput;
	#xorOutput;

	constructor(polynomial, initial_value, reflectInput, reflectOutput, xorOutput) {
		this.#poly = polynomial;
		this.#initial = initial_value ?? 0;
		this.#reflectInput = reflectInput ?? false;
		this.#reflectOutput = reflectOutput ?? false;
		this.#xorOutput = xorOutput ?? 0x00;
	}

	checksum(bytes, length) { return this.#doChecksum8(bytes, length ?? bytes.byteLength, this.#poly, this.#initial, this.#reflectInput, this.#reflectOutput, this.#xorOutput); }
	#doChecksum8(bytes, length, poly, initial, refIn, refOut, xorOut) @ "xs_checksum8";
}

class CRC16 {
	#poly;
	#initial;
	#reflectInput;
	#reflectOutput;
	#xorOutput;

	constructor(polynomial, initial_value, reflectInput, reflectOutput, xorOutput) {
		this.#poly = polynomial;
		this.#initial = initial_value ?? 0;
		this.#reflectInput = reflectInput ?? false;
		this.#reflectOutput = reflectOutput ?? false;
		this.#xorOutput = xorOutput ?? 0x00;
	}

	checksum(bytes, length) { return this.#doChecksum16(bytes, length ?? bytes.byteLength, this.#poly, this.#initial, this.#reflectInput, this.#reflectOutput, this.#xorOutput); }
	#doChecksum16(bytes, length, poly, initial, refIn, refOut, xorOut) @ "xs_checksum16";
}

export { CRC8 as default, CRC8, CRC16 };
