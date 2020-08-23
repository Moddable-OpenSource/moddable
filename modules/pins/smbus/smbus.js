/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
	SMBus
	
	builds on I2C in Pins to provide SMBus functions like Kinoma Create and Kinoma Element
*/

import I2C from "pins/i2c";

export default class SMBus extends I2C {
	readByte(register) {
		super.write(register, false);			// set address
		return super.read(1)[0];				// read one byte
	}
	readWord(register, endian) {
		super.write(register, false);			// set address
		const value = super.read(2);			// read two bytes
		return endian ? (value[1] | (value[0] << 8)) : (value[0] | (value[1] << 8));
	}
	readBlock(register, count, buffer) {
		super.write(register, false);			// set address
		return buffer ? super.read(count, buffer) : super.read(count);
	}
	writeByte(register, value) {
		return super.write(register, value & 255);
	}
	writeWord(register, value, endian) {
		if (endian)
			return super.write(register, (value >> 8) & 255, value & 255);
		return super.write(register, value & 255, (value >> 8) & 255);
	}
	writeBlock(register, ...value) {
		return super.write(register, ...value);
	}
}
