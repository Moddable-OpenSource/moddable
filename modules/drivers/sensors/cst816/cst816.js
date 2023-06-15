/*
 * Copyright (c) 2019-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

/*
	ToDo: Add Gesture and Proximity
 */

import Timer from "timer";

const WORK_MODE_REG = 0x00;
	/* R-- 00: NORMAL, 04: DBG_IDAC, E0: DBG_POS, 06: DBG_RAW, 07: DBG_SIG */
const PROXIMITY_ID_REG = 0x01;
	/* R-- 00: default, C0: Far away, E0: near */
const TOUCH_NUM_REG = 0x02;
	/* R-- bit 7~4: ??, 3~0: touch points [3:0]					*/
const TOUCH_DATA_HIGH = 0x03;
	/* R-- bit 7~6: event_flg, 5~4: ??, 3~0: X_pos High [11:8]	*/
const TOUCH_DATA_LOW = 0x04;
	/* R-- bit 7~0: X_pos LOW [7:0]	*/
const SLEEP_REG = 0xA5;
	/* write 0x03 into deepsleep */


class CST816  {
	#io;

	constructor(options) {
		const {sensor, reset, interrupt, onSample, target} = options;
		const io = this.#io = new sensor.io({
			hz: 100_000,
			address: 0x15,
			...sensor
		});
		
		try {
			io.buffer = new Uint8Array(7);		// one point

			if (reset) {
				io.reset = new reset.io({
					...reset
				});

				io.reset.write(0);
				Timer.delay(5);
				io.reset.write(1);
				Timer.delay(150);
			}

			try {
				let check = io.readUint16(0xA8);
			}
			catch {};

			if (interrupt && onSample) {
				io.interrupt = new interrupt.io({
					...interrupt,
					edge: interrupt.io.Falling,
					onReadable: onSample.bind(this)
				});
			}
			if (target)
				this.target = target;

			this.configure({
				active: false,
				timeout: 10
			});
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	close() {
		this.#io?.reset?.close();
		this.#io?.interrupt?.close();
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		const io = this.#io;

		let value = options.flip;
		if (value) {
			delete io.flipX;
			delete io.flipY;

			if ("h" === value)
				io.flipX = true; 
			else if ("v" === value)
				io.flipY = true; 
			else if ("hv" === value)
				io.flipX = io.flipY = true; 
		}

		if (false === options.active)
			io.writeUint8(SLEEP_REG, 0x03);	// put device to sleep

		io.length = 1;

	}
	get configuration() {
		return {
			interrupt: !!this.#io.interrupt
		}
	}
	sample() {
		const io = this.#io;

		const data = io.buffer;
		io.readBuffer(WORK_MODE_REG, data);

		const length = data[TOUCH_NUM_REG] & 0x0f;
		if (0 === length) {
			if (io.none)
				return;
			io.none = true;
			return [];
		}
		delete io.none;

		const result = new Array(length);
		for (let i = 0; i < length; i++) {
			const offset = (i * 2) + 3;
			const evt = (data[offset] >> 6) & 0b11;
			let x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
			const id = (data[offset + 2] >> 4) & 0b1111;
			let y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];

			if (io.flipX)
				x = screen.width ? (screen.width - x) : x;	//@@ screen.width?

			if (io.flipY)
				y = screen.height ? (screen.height - y) : y;	//@@ screen.height?

			const j = {x, y, evt};

			if (1 === io.length) {
				j.id = 0;
				result[0] = j;
				break;
			}

			j.id = id;
			result[i] = j;
		}

		return result;
	}
}

export default CST816;
