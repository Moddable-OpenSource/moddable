/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import Timer from "timer";

class GT911 {
	#io;

	constructor(options) {
		const {sensor, interrupt, onSample, config} = options;
		const io = this.#io = new sensor.io({
			hz: 200_000,		// ....data sheet warns about speeds over 200_000
			address: 0x5D,
			...sensor
		});

		// check id
		io.write(Uint8Array.of(0x81, 0x40));
		const data = new Uint8Array(io.read(3));
		if ((57 !== data[0]) || (49 !== data[1]) || (49 !== data[2]))
			throw new Error("unrecognized");

		// set-up interrupt
		if (interrupt && onSample) {
			io.interrupt = new interrupt.io({
				...interrupt,
				edge: interrupt.io.Falling,
				onReadable: onSample.bind(this)
			});
		}

		// program configuration
		if (config) {
			const start = 0x8047, end = 0x80FF;
			if (config.length !== (end - start))
				throw new Error("bad config");

			// see if configuration needs update
			let update = false;
			for (let address = start, step = 16; address < end; address += step) {
				let use = step;
				if ((address + use) > end)
					use = end - address;
				io.write(Uint8Array.of(address >> 8, address & 0xff));
				const data = new Uint8Array(io.read(use));
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

					io.write(Uint8Array.of(address >> 8, address & 0xff, slice));

					for (let i = 0; i < use; i++)
						checksum += slice[i];
				}
				checksum = ((~checksum) + 1) & 0xFF;
				io.write(Uint8Array.of(end >> 8, end & 0xff, checksum, 1));

				Timer.delay(100);
			}
		}

		io.write(Uint8Array.of(0x81, 0x4E, 0));		// ready for next reading
	}
	close() {
		this.#io?.close();
		this.#io.interrupt?.close();
		this.#io = undefined;
	}
	configure() {
		//@@ set length...
		//@@ other config?
	}
	sample() {
		const io = this.#io;

		io.write(Uint8Array.of(0x81, 0x4E)); 		// GOODIX_READ_COOR_ADDR
		const status = (new Uint8Array(io.read(1)))[0];
		if (!(0x80 & status))	// not-ready
			return;

		const touchCount = status & 0b0000_1111;
		if (!touchCount) {
			delete this.#io.last;
			io.write(Uint8Array.of(0x81, 0x4E, 0));	// ready for next reading
			return [];
		}

		const data = this.#io.last = new Uint8Array(touchCount << 3);
		io.read(data);

		const result = new Array(touchCount);
		for (let i = 0; i < touchCount; i++) {
			const offset = i * 8;
			const id = data[offset];

			const x = (data[offset + 1] | (data[offset + 2] << 8));
			const y = (data[offset + 3] | (data[offset + 4] << 8));
			const size = (data[offset + 5] | (data[offset + 6] << 8));

			result[i] = {x, y, id, size};
		}

		io.write(Uint8Array.of(0x81, 0x4E, 0));	// ready for next reading

		return result;
	}
}

export default GT911;
