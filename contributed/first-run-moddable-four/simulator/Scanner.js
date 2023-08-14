/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

export default class Scanner {
	constructor(target) {
		this.discoveries = new Map();
		this.timer = Timer.repeat(() => {
			if (this.discoveries.size < 20) {
				let address = "";
				for (let i = 0; i < 6; i++) {
					if (i > 0)
						address += ":";
					const value = Math.round(Math.random() * 255);
					if (value < 16)
						address += "0";
					address += value.toString(16).toUpperCase();
				}
				let rssi = -30 - Math.round(Math.random() * 60);
				this.discoveries.set(address, { address, rssi });
				target.defer("onDiscovered", this.discoveries);
			}
		}, 500);
	}
	close() {
		Timer.clear(this.timer);
	}
}
