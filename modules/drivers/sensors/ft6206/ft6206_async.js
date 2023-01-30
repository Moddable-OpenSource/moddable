/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
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

import Timer from "timer";

class FT6206  {
	#io;
	#timer;
	#onError;
	#onSample;

	constructor(options) {
		const {sensor, reset, interrupt, onSample, target, onError} = options;
		const io = this.#io = new sensor.io.Async({
			hz: 100_000,
			address: 0x38,
			...sensor
		});
		this.#onError = onError;
		this.#onSample = onSample;
		io.buffer = new Uint8Array(12);		// two touch points
		io.configure = {
			active: false,
			threshold: 128,
			timeout: 10
		};

		if (interrupt && onSample) {
			io.interrupt = new interrupt.io({
				...interrupt,
				edge: interrupt.io.Falling,
				onReadable: () => this.#doSample()
			});
		}
		if (target)
			this.target = target;

		const check = () => {
			this.#timer = undefined;
			this.#io.readUint8(0xA8, (error, value) => {
				if (error)
					this.#onError?.(error);
				else if (17 !== value)
					this.#onError?.("unexpected vendor");
				else {
					this.#io.readUint8(0xA3, (error, id) => {
						if (error)
							this.#onError?.(error);
						else if ((6 !== id) && (100 !== id))
							this.#onError?.("unexpected chip");
						else {
							const configure = this.#io.configure;
							const timeout = configure.timeout;
							delete configure.timeout;
							delete this.#io.configure;
							this.configure(configure);
							io.writeUint8(0x87, timeout, () => {
								if (io.interrupt)
									return;		// the interrupt could fire before this.... should probably ignore it?

								this.#timer = Timer.set(() => {
									this.#doSample();
								}, 0, 33);
							});
						}
					});
				}
			})
		};

		if (reset) {
			io.reset = new reset.io({
				...reset
			});

			io.reset.write(0);
			Timer.delay(5);
			io.reset.write(1);
			this.#timer = Timer.set(check, 150);
		}
		else
			check();
	}
	close(callback) {
		this.#io?.reset?.close();
		this.#io?.interrupt?.close();
		if (callback && this.#io)
			this.#io.close(error => callback(error));
		this.#io = undefined;
		Timer.clear(this.#timer);
		this.#timer = undefined;
	}
	configure(options) {
		const io = this.#io;
		if (io.configure) {
			io.configure = {...options, ...io.configure};
			return;
		}

		if ("threshold" in options)
			io.writeUint8(0x80, options.threshold);

		if ("active" in options)
			io.writeUint8(0x86, options.active ? 0 : 1);

		if ("timeout" in options)
			io.writeUint8(0x87, options.timeout);

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

		value = options.length;
		if (undefined !== value)
			io.length = (1 === value) ? 1 : 2; 

		if ("weight" in options) {
			delete io.weight;
			if (options.weight)
				io.weight = true;
		}

		if ("area" in options) {
			delete io.area;
			if (options.area)
				io.area = true;
		}
	}
	sample() {
		const result = this.#io.sample;
		delete this.#io.sample; 
		return result;
	}
	#doSample() {
		if (this.#timer)
			Timer.schedule(this.#timer);
		this.#io.readUint8(0x02, (error, length) => {
			length &= 0x0F;			// number of touches
			if (!length) {
				if (!this.#io.interrupt)
					Timer.schedule(this.#timer, 17, 17);
				if (this.#io.none)
					return;
				this.#io.sample = [];
				this.#io.none = true;
				this.#onSample?.();
				return;
			}
			delete this.#io.none;
			this.#io.readBuffer(0x03, this.#io.buffer, () => {
				const io = this.#io, data = io.buffer;
				const result = new Array(length);
				for (let i = 0; i < length; i++) {
					const offset = i * 6;
					const id = data[offset + 2] >> 4;
					if (id && (1 === io.length))
						continue;

					let x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
					let y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];

					if (io.flipX)
						x = 240 - x;

					if (io.flipY)
						y = 320 - y;

					result[i] = {x, y, id};

					if (io.weight)
						result[i].weight = data[offset + 4];

					if (io.area)
						result[i].area = data[offset + 5] >> 4;
				}

				io.sample = result;

				if (!io.interrupt)
					Timer.schedule(this.#timer, 17, 17);
				this.#onSample?.();
			});
		});
	}
}

export default FT6206;
