/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import {} from "_262";
import {} from "harness";
import ChecksumOut from "commodetto/checksumOut";
import Timer from "timer";
import WiFi from "wifi";
import Net from "net";
import config from "mc/config";

globalThis.$DO = function(f) {
	return function() {
		try {
			f();
			$DONE();
		}
		catch(e) {
			$DONE(e);
		}
	}
}

class Screen extends ChecksumOut {
	#timer;

	clear() {}
	start(interval) {
		if (!this.#timer) {
			this.#timer = Timer.set(() => {
				this.context.onIdle();
			}, 1, 100);
			Timer.schedule(this.#timer);
		}

		const timer = this.#timer;
		if (!timer) return;

		if (interval <= 5)
			interval = 5;
		if (timer.interval === interval)
			return;

		Timer.schedule(timer, interval, interval);
		timer.interval = interval;
	}
	stop() {
		const timer = this.#timer;
		if (!timer) return;

		Timer.schedule(timer);
		delete timer.interval;
	}
	animateColors(clut) {}
	checkImage(checksum, message) {
		delete this.checksum;
		this.context.onIdle();
		assert.sameValue(checksum, this.checksum, message ?? "image mismatch");
	}
}


Object.defineProperty(globalThis, "screen", {
	enumerable: true,
	configurable: true,
	get() {		// instantiate screen on first access
		let value, width, height;
		
		if (config.Screen) { 
			value = new config.Screen({});
			width = value.width,
			height = value.height;
		}
		Object.defineProperty(globalThis, "screen", {
			enumerable: false,
			configurable: true,
			writable: true,
			value
		});

		screen = new Screen({width: width ?? 240, height: height ?? 320});
		screen.configure({show: true});

		return screen;
	},
	set(value) {	// simulator creates screen
		Object.defineProperty(globalThis, "screen", {
			enumerable: false,
			configurable: true,
			writable: true,
			value
		});

		screen = new Screen({width: value.width, height: value.height});

		return screen;
	}
});

globalThis.$NETWORK = {
	get connected() {
		if (1 !== WiFi.mode)
			WiFi.mode = 1;

		if (Net.get("IP"))
			return Promise.resolve();

		assert(!!config.ssid, "Wi-Fi SSID missing");
		return new Promise((resolve, reject) => {
			trace(`Connecting to Wi-Fi SSID "${config.ssid}"...\n`);
			new WiFi({ssid: config.ssid, password: config.password}, function(message) {
				if (WiFi.gotIP === message) {
					trace(`...connected.\n`);
					resolve();
					this.close();
				}
			});
		});
	},
	async wifi(options) {
		// could be async to allow time to bring up an AP 
		return {ssid: config.ssid, password: config.password};
	},
	invalidDomain: "fail.moddable.com",
};

class HostObject @ "xs_hostobject_destructor" {
	constructor() @ "xs_hostobject"
}

class HostObjectChunk @ "xs_hostobjectchunk_destructor" {
	constructor() @ "xs_hostobjectchunk"
}

class HostBuffer @ "xs_hostbuffer_destructor" {
	constructor() @ "xs_hostbuffer"
}

class TestBehavior extends Behavior {
	constructor() {
		super();

		for (const name of Object.getOwnPropertyNames(Object.getPrototypeOf(this))) {
			if (!name.startsWith("on")) continue;
			const value = this[name];
			Object.defineProperty(this, name, {
				value: (application, data) => {try {return value.call(this, application, data);} catch (e) {$DONE(e)}}
			});
		}
	}
}

globalThis.$TESTMC = {
	timeout(ms, message = "timeout") {
		Timer.set(function() {
			$DONE(message);
		}, ms);
	},
	HostObject,
	HostObjectChunk,
	HostBuffer,
	config,
	wifiInvalidConnectionTimeout: 20_000,
	wifiConnectionTimeout: 10_000,
	wifiScanTimeout: 10_000,
	Behavior: TestBehavior
};

Object.freeze([globalThis.$TESTMC, globalThis.$NETWORK], true);

export default function() {
	$MAIN();
}
