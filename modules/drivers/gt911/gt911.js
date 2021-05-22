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

import I2C from "pins/i2c";
import Time from "time";
import Timer from "timer";

class GT911 extends I2C {
	#until;
	#data;

	constructor(dictionary) {
		super({
			address: 0x5D,
			hz: 200_000,		// ....data sheet warns about speeds over 200_000
			...dictionary
		});

		// check id
		super.write(0x81, 0x40);
		const data = super.read(3);
		const id = String.fromCharCode(data[0]) + String.fromCharCode(data[1]) + String.fromCharCode(data[2]);
		if ("911" !== id)
			throw new Error("unrecognized");

		if (dictionary?.config) {
			const start = 0x8047, end = 0x80FF;
			const config = dictionary.config;
			if (config.length !== (end - start))
				throw new Error("bad config");

			// see if configuration needs update
			let update = false;
			for (let address = start, step = 16; address < end; address += step) {
				let use = step;
				if ((address + use) > end)
					use = end - address;
				super.write(address >> 8, address & 0xff);
				const data = super.read(use);
				for (let i = 0, offset = address - start; i < use; i++, offset++) {
					if (data[i] !== config[offset])
						update = true;
				}
			}

			if (update) {	// write configuration
				let checksum = 0;
				for (let address = start, step = 16, offset = 0; address < end; address += step, offset += step) {
					let use = step;
					if ((address + use) > end)
						use = end - address;
					const slice = new  Uint8Array(use);
					for (let i = 0; i < use; i++)
						slice[i] = config[offset + i];

					super.write(address >> 8, address & 0xff, slice);

					for (let i = 0; i < use; i++)
						checksum += slice[i];
				}
				checksum = ((~checksum) + 1) & 0xFF;
				super.write(end >> 8, end & 0xff, checksum, 1);

				Timer.delay(100);
			}
		}

		super.write(0x81, 0x4E, 0);		// ready for next reading
	}

	read(target) {
		this.write(0x81, 0x4E); 		// GOODIX_READ_COOR_ADDR
		let data = this.#data;
		const byteLength = 1 + (target.length) * 8;
		if (!data || (data.byteLength !== byteLength))
			this.#data = data = new Uint8Array(byteLength);
		super.read(byteLength, data.buffer);

		if (!(data[0] & 0x80) && (0x7F & target[0].state) && (Time.ticks < this.#until)) {
			for (let i = 0; i < target.length; i++)
				target[i].state |= 0x80;
			return;						// can take 10 to 15 ms for the next reading to be available
		}

		if (data[0] & 0x80)	 {						// down
			super.write(0x81, 0x4E, 0);				// ready for next reading
			this.#until = Time.ticks + 15;			// latest time next reading should be available, otherwise touch up
		}

		const count = data[0] & 0x0F;
		const end = 1 + (8 * count);
points:
		for (let i = 0; i < target.length; i++) {
			const point = target[i];
			const state = point.state & 0x7F;

			for (let offset = 1; offset < end; offset += 8) {
				const id = data[offset];
				if (id !== i)
					continue;

				// contact
				point.x = (data[offset + 2] << 8) | data[offset + 1];
				point.y = (data[offset + 4] << 8) | data[offset + 3];

				if ((0 === state) || (3 === state))		// nothing or up
					point.state = 1;					//  -> down
				else									// down or move
					point.state = 2;					//  -> move
				continue points;
			}

			// no contact
			if ((1 === state) || (2 === state))		// down or moved
				point.state = 3;					//  -> up
			else									// nothing or up
				point.state = 0;					//  -> nothing
		}
	}
}

export default GT911;
