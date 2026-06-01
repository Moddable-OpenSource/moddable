/*
 * Copyright (c) 2019-2026  Moddable Tech, Inc.
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

// https://datasheet4u.com/datasheets/Hynitron/CST328/1577247

/*
	ToDo: Add Gesture and Proximity
 */

import Timer from "timer";

const CST328_MAX_POINTS = 5;
const CST328_ADDR		= 0x1A;

const CST328_1ST_TOUCH_ID = 0xD000;
const CST328_1ST_TOUCH_XH = 0xD001;
const CST328_1ST_TOUCH_YH = 0xD002;
const CST328_1ST_TOUCH_XLYL = 0xD003;
const CST328_1ST_TOUCH_PRESSURE = 0xD004;
const CST328_TOUCH_FLAG_AND_NUM = 0xD005;
const CST328_ENUM_MODE_NORMAL = 0xD109;

class CST328  {
	#io;

	constructor(options) {
		const {sensor, reset, interrupt, onSample, target} = options;
		const io = this.#io = new sensor.io({
			hz: 100_000,
			address: CST328_ADDR,
			...sensor
		});
		
		try {
			io.flag = new Uint8Array(1);
			io.buffer = new Uint8Array(27);		// five points

			if (reset) {
				io.reset = new reset.io({
					...reset,
					initialValue: 0
				});

				Timer.delay(10);
				io.reset.write(1);
				Timer.delay(50);
			}

			try {
				io.writeUint8(CST328_ENUM_MODE_NORMAL, 0);
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

//		if (false === options.active)
//			io.writeUint8(SLEEP_REG, 0x03);	// put device to sleep

//		io.length = 1;

	}
	get configuration() {
		return {
			interrupt: !!this.#io.interrupt
		}
	}
	sample() {
		const io = this.#io;

		const flag = io.flag;
		io.readBuffer(CST328_TOUCH_FLAG_AND_NUM, flag);
		const length = flag & 0x0f;

		if (0 === length) {
			if (io.none)
				return;
			io.none = true;
			return [];
		}
		delete io.none;

		const data = io.buffer;
		io.readBuffer(CST328_1ST_TOUCH_ID, data);
		if ((data[0] & 0x0f) != 0x06) { // not pressed
			io.none = true;
			return [];
		}

		const result = new Array(length);
		for (let i = 0; i < length; i++) {
			const offset = (i * 5) + ((i > 0) * 2);

			const id = data[offset] >> 4;
			let x = (data[offset + 1] << 4) + ((data[offset + 3] & 0xF0) >> 4);
			let y = (data[offset + 2] << 4) + (data[offset + 3] & 0x0F);
			const pressure = data[offset + 4];

			if (io.flipX)
				x = screen.width ? (screen.width - x) : x;	//@@ screen.width?

			if (io.flipY)
				y = screen.height ? (screen.height - y) : y;	//@@ screen.height?

			const j = {x, y, pressure};

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

export default CST328;
