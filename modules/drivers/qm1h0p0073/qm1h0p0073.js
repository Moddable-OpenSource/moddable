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
	Sharp qm1h0p0073 humidity and temperature sensor
*/

import I2C from "pins/i2c";
import Timer from "timer";

class QM1H0P0073 extends I2C {
	constructor(dictionary) {
		super(Object.assign({address: 0x28}, dictionary));
	}

	configure(dictionary) {
		throw new Error("no configurable properties");
	}

	sample() {
		this.write();				// measurement request (latch new readings)
		Timer.delay(50);
		let bytes = this.read(4);	// data fetch (read latched measurements)
		return {
			humidity: ((((bytes[0] << 8) | bytes[1]) >> 2) / 16384) * 100,
			temperature: (((((bytes[2] << 8) | bytes[3]) >> 2) / 16384) * 165) - 40
		}
	}
}

export {QM1H0P0073 as default, QM1H0P0073};





