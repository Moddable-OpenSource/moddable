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

/*
	ToDo: Add Gesture and Proximity
 */

import Timer from "timer";

const WORK_MODE_REG = 0x00;
const PROXIMITY_ID_REG = 0x01;
const TOUCH_NUM_REG = 0x02;
const TOUCH_DATA_HIGH = 0x03;
const TOUCH_DATA_LOW = 0x04;
const SLEEP_REG = 0xA5;
const IRQ_CTL = 0xFA;
const IRQ_PULSE_WIDTH = 0xED;

class CST816  {
	#io;

	constructor(options) {
		const {sensor, reset, interrupt, onSample, target} = options;
		const io = this.#io = new sensor.io({
			hz: 400_000,
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
				this.#io
				io.writeUint8(IRQ_CTL, 0x41);	// Enable Touch
				io.writeUint8(IRQ_PULSE_WIDTH, 0x01);	// 0.1ms
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

		let length = data[TOUCH_NUM_REG];
		if (0 === length) {
			if (io.none)
				return;
			io.none = true;
			return [];
		}
		delete io.none;

		const result = new Array(1);
		const evt = data[1];
		let x = ((data[3] & 0x0F) << 8) | data[4];
		let y = ((data[5] & 0x0F) << 8) | data[6];

		if (io.flipX)
			x = screen.width ? (screen.width - x) : x;	//@@ screen.width?

		if (io.flipY)
			y = screen.height ? (screen.height - y) : y;	//@@ screen.height?

		const j = {id: 0, x, y, evt};
		result[0] = j;

		return result;
	}
}

export default CST816;
