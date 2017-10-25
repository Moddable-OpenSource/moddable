/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	
	N.B. readBlockDataSMB always returns an Array and writeBlockDataSMB only accepts Number and String value arguments
*/

import I2C from "pins/i2c";

export default class SMBus extends I2C {
	readByteDataSMB(register) {
		this.write(register);				// set address
		return this.read(1)[0];				// read one byte
	}
	readWordDataSMB(register) {
		this.write(register);				// set address
		let value = this.read(2);			// read two bytes
		return value[0] | (value[1] << 8);
	}
	readBlockDataSMB(register, count, buffer) {
		this.write(register);				// set address
		return buffer ? this.read(count, buffer) : this.read(count);
	}
	writeByteDataSMB(register, value) {
		this.write(register, value & 255);
	}
	writeWordDataSMB(register, value) {
		this.write(register, value & 255, (value >> 8) & 255);
	}
	writeBlockDataSMB(register, ...value) {
		this.write(register, ...value);
	}
}
