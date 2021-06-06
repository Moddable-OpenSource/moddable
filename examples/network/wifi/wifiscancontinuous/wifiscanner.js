/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import WiFi from "wifi";
import Time from "time";
import Timer from "timer";

class Scanner {
	#items = [];
	#interval;
	#max;
	#scanOptions;
	#onLost;
	#onFound;
	#onScanning;
	#timer;

	constructor(options) {
		if (undefined !== options.target)
			this.target = options.target;
		this.#onFound = options.onFound;
		this.#onLost = options.onLost;
		this.#onScanning = options.onScanning;
		this.#max = options.max ?? 32;
		this.#interval = options.interval ?? 1000;
		this.#scanOptions = options.scanOptions ?? {};

		this.#timer = Timer.set(() => {
			this.#timer = undefined;
			this.#scan();
		});
	}
	#scan() {
		WiFi.scan(this.#scanOptions, item => {
			if (!this.#items)
				return;

			if (item) {
				const i = this.#items.find(i => i.ssid === item.ssid);
				if (i)
					i.ticks = Time.ticks;
				else {
					const result = this.#onFound?.(item);
					if ((undefined === result) || result) {
						this.#items.push({ssid: item.ssid, ticks: Time.ticks});
						if (this.#items.length > this.#max)
							this.#purge();
					}
				}
			}
			else {
				this.#purge();
				this.#timer = Timer.set(() => {
					this.#timer = undefined;
					this.#scan();
				}, this.#interval);
				this.#onScanning?.(false);
			}
		});
		this.#onScanning?.(true);
	}
	#purge() {
		const items = this.#items;
		let expire = Time.ticks - 60 * 1000;		// haven't been seen in 60 seconds? gone.
		if (expire < 0) expire = 0;
		for (let i = 0; i < items.length; i++) {
			if (items[i].ticks > expire)
				continue;

			this.#onLost?.(items[i].ssid);

			items.splice(i, 1);
			i -= 1;
		}

		if (items.length > this.#max) {
			items.sort((a, b) => b.ticks - a.ticks);		// most recently seen first
			if (this.#onLost) {
				for (let i = this.#max; i < items.length; i++)
					this.#onLost(items[i].ssid)
			}
			items.length = this.#max;
		}
	}
	close() {
		if (this.#timer)
			Timer.clear(this.#timer);
		this.#timer = undefined;
		this.#items = undefined;
	}
}
Object.freeze(Scanner.prototype);

export default Scanner;
