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

import I2C from "pins/i2c";
import Time from "time";

class GT911 extends I2C {
	constructor(dictionary) {
		super(Object.assign({address: 0x14}, dictionary));

		// check id
		super.write(0x81, 0x40);
		let data = super.read(6);
		let id = String.fromCharCode(data[0]) + String.fromCharCode(data[1]) + String.fromCharCode(data[2]);
		if ("911" !== id)
			throw new Error("unrecognized");

		// check configuration
		super.write(0x80, 0x47);
		data = super.read(40);		//@@ 186

		let view = new DataView(data.buffer);
		this.maxX = view.getInt16(1, true);
		this.maxY = view.getInt16(3, true);
		this.maxTouch = view.getUint8(5) & 0x0F;

		super.write(0x81, 0x4E, 0);		// ready for next reading
	}

	read(target) {
		this.write(0x81, 0x4E); 		// GOODIX_READ_COOR_ADDR
		let data = super.read(9);

		if (!(data[0] & 0x80) && target[0].state && (Time.ticks < this.until))
			return;						// can take 10 to 15 ms for the next reading to be available

		let count = data[0][0] & 0x0F;
		target[0].x = (data[3] << 8) | data[2];
		target[0].y = (data[5] << 8) | data[4];

		if (data[0] & 0x80) { // down
			// down
			super.write(0x81, 0x4E, 0);		// ready for next reading
			this.until = Time.ticks + 15;	// latest time next reading should be available, otherwise touch up

			switch (target[0].state) {
				case 0:
				case 1:
					target[0].state += 1;
					break;
				case 2:
				case 3:
					break;
			}
		}
		else { // up
			if ((1 === target[0].state) || (2 === target[0].state))
				target[0].state = 3;
			else
				target[0].state = 0;
		}
	}
}
Object.freeze(GT911.prototype);

export default GT911;
