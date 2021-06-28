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


class CRC8  @ "xs_crc8_destructor" {
	constructor(polynomial, initial, reflectInput, reflectOutput, xorOutput) @ "xs_crc8";
	close() @ "xs_crc8_close";
	checksum(buffer) @ "xs_crc8_checksum";
	reset() @ "xs_crc8_reset";
}

class CRC16  @ "xs_crc16_destructor" {
	constructor(polynomial, initial, reflectInput, reflectOutput, xorOutput) @ "xs_crc16";
	close() @ "xs_crc16_close";
	checksum(buffer) @ "xs_crc16_checksum";
	reset() @ "xs_crc16_reset";
}

export { CRC8 as default, CRC8, CRC16 };
