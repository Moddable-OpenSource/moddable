/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
	Texas Instruments TMP102 sensor - http://www.ti.com/lit/ds/symlink/tmp102.pdf
*/

import SMBus from "pins/smbus";
import Timer from "timer";

class TMP102 extends SMBus {
	constructor(dictionary) {
		super(Object.assign({address: 0x48}, dictionary));
		super.writeBlock(3, 0x7f, 0xf8);		// disable alert
		// Configuration register is 0xA060 on reset as per data sheet
		const config = super.readWord(1);
		if (!(0x1000 & config)) {
			super.writeWord(1, super.readWord(1) | 0x1000);					// turn on 13-bit range (Extended Mode - EM bit)
			Timer.delay(250);												// wait for a new reading to be available, so it is at the correct resolution
		}
	}

	configure(dictionary) {
		if (dictionary.alert) {
			const alert = dictionary.alert;
			if (alert.above) {
				const value = (alert.above / 0.0625) << 3;					// 13-bits
				super.writeBlock(3, value >> 8, value & 0xff);				// write Temperature Hi register
			}
			else
				super.writeBlock(3, 0x7f, 0xf8);							// write Temperature Hi register (max value)
		}
		if (dictionary.hz) {
			let hz = dictionary.hz;
			if (hz < 0.5)
				hz = 0b00;
			else if (hz < 2)
				hz = 0b01;
			else if (hz < 6)
				hz = 0b10;
			else
				hz = 0b11;
			super.writeWord(1, (super.readWord(1) & ~0xC000) | (hz << 14));
		}
	}

	sample() {
		let value = super.readWord(0);		// 13-bits
		value = ((value & 0x00FF) << 5) | ((value & 0xFF00) >> 11);
		if (value & 0x1000) {
			value -= 1;
			value = ~value & 0x1FFF;
			value = -value;
		}
		return {temperature: value * 0.0625};
	}
}
Object.freeze(TMP102.prototype);

export default TMP102;
