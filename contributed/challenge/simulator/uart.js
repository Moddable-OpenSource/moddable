/*
 * Copyright (c) 2016-2024 Moddable Tech, Inc.
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
		this.index = 0;
		this.target = target;
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onConnected();
		}, 2000);
	}
	close() {
		if (this.timer)
			Timer.clear(this.timer);
	}
	onConnected() {
		this.target.defer("onConnected");
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onReceived();
		}, 2000);
	}
	onDisconnected() {
		this.target.defer("onDisconnected");
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onConnected();
		}, 2000);
	}
	onReceived() {
		this.index++;
		if (this.index == 10)
			this.index = 0;
		if (0) {
			this.target.defer("onReceived", `
function* test() {
  guess("${ this.index.toString().repeat(10) }");
  yield;
}
`);
		}
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onDisconnected();
		}, 2000);
	}
	transmit(value) {
		trace(value + "\n");
	}
}
