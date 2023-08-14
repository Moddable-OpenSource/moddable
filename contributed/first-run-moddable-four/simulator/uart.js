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

export default class UARTServer {
	constructor(target) {
		this.target = target;
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onPasskey();
		}, 2000);
	}
	close() {
		if (this.timer)
			Timer.clear(this.timer);
	}
	onConnected() {
		this.target.defer("onConnected");
	}
	onPasskey() {
		let passkey = Math.round(Math.random() * 999999);
		this.target.defer("onPasskey", passkey.toString().padStart(6, "0"));
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onConnected();
		}, 2000);
	}
	transmit(event) {
	}
}
