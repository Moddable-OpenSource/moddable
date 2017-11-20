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
	Sharp GP2AP01VT00F TOF sensor
*/

import SMBus from "pins/smbus";
import Timer from "timer";

const Emitter = {
	mA_20: 0b0100,
	mA_25: 0b0101,
	mA_29: 0b0110,
	mA_33: 0b0111,
	mA_37: 0b1000,
	mA_41: 0b1001,
	mA_45: 0b1010,
	mA_48: 0b1011,
	mA_51: 0b1100,
	mA_54: 0b1101,
	mA_57: 0b1110,
	mA_59: 0b1111,
};
Object.freeze(Emitter);

const Interval = {
	ms_000: 0x00,
	ms_001: 0x01,
	ms_002: 0x02,
	ms_003: 0x03,
	ms_004: 0x04,
	ms_041: 0x05,
	ms_082: 0x06,
	ms_165: 0x07
}
Object.freeze(Interval);

class GP2AP01VT00F extends SMBus {
	constructor(dictionary) {
		super(Object.assign({address: 0x29}, dictionary));

		if (0x0F != this.readByte(0x3B))
			throw new Error("unrecognized ID");

		this.writeByte(0x00, 0x00);					// setting for shutdown mode
		this.writeByte(0x02, 0x12);					// interrupt type : pulse
		this.writeByte(0x03, Interval.ms_000);		// interval time
		this.writeByte(0x04, 0xE0 | Emitter.mA_59);	// vcsel current
		this.writeByte(0x05, 0x1F);					// setting of circuit
		this.writeByte(0x06, 0x0F);					// mode etc
		this.writeByte(0x07, 0xB3);
		this.writeByte(0x08, 0x00);
		this.writeByte(0x0A, 0x11);
		this.writeByte(0x0B, 0x06);
		this.writeByte(0x0D, 0x34);					// convergence time
		this.writeByte(0x46, 0x74);
		this.writeByte(0x47, 0x5F);

		this.writeByte(0x00, 0xC0);					// continuous operation mode
		Timer.delay(2);   // wait 2 msec.
	}

	close() {
		this.writeByte(0x00, 0);						// shutdown mode
		super.close();
	}

	configure(dictionary) {
		this.writeByte(0x00, 0);						// shutdown mode

		for (let property in dictionary) {
			switch (property) {
				case "interval":
					this.writeByte(0x03, parseInt(dictionary[property]));
					break;
				case "emitter":
					this.writeByte(0x04, 0xE0 | parseInt(dictionary[property]));
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}

		this.writeByte(0x00, 0xC0);					// continuous operation mode
		Timer.delay(2);   // wait 2 msec.
	}

	sample() {
		let c1 = this.readWord(0x52);				// read c1 and c2
		c1 = ((c1 & 0xFF) << 8) | (c1 >> 8);
		let bytes = this.readBlock(0x0E, 3);
		let range = bytes[0] | (bytes[1] << 8) | ((bytes[2] & 0x0F) << 16);
		return {distance: 0.00653 * (range - c1)};
	}
}
Object.freeze(GP2AP01VT00F.prototype);

export {GP2AP01VT00F as default, GP2AP01VT00F, Emitter, Interval};
