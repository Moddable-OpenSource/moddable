/*
 * Copyright (c) 2018-2026  Moddable Tech, Inc.
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
import Modules from "modules";
import ChecksumOut from "commodetto/checksumOut";
import Timer from "timer";
import config from "mc/config";
import { URL, URLSearchParams } from "url";
globalThis.URL = URL;
globalThis.URLSearchParams = URLSearchParams;

globalThis.$DO = function(f) {
	return function(...args) {
		try {
			f(...args);
			$DONE();
		}
		catch(e) {
			$DONE(e);
		}
	}
}

class Screen extends ChecksumOut {
	#timer;
	#promises = [];

	clear() {}
	start(interval) {
		if (!this.#timer) {
			this.#timer = Timer.set(() => {
				this.doIdle();
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
		this.doIdle();
		assert.sameValue(checksum, this.checksum, message ?? "image mismatch");
	}
	doIdle() {
		this.context.onIdle();
		const promises = this.#promises;
		this.#promises = [];
		for (let i = 0; i < promises.length; i++)
			promises[i]();
	}
	doTouchBegan(id, x, y, ticks) {
		return new Promise((resolve, reject) => {
			Timer.set(() => {
				this.context.onTouchBegan(id, x, y, ticks);
				this.#promises.push(resolve);
				this.doIdle();
			});
		});
	}
	doTouchMoved(id, x, y, ticks) {
		return new Promise((resolve, reject) => {
			Timer.set(() => {
				this.context.onTouchMoved(id, x, y, ticks);
				this.#promises.push(resolve);
			});
		});
	}
	doTouchEnded(id, x, y, ticks) {
		return new Promise((resolve, reject) => {
			Timer.set(() => {
				this.context.onTouchEnded(id, x, y, ticks);
				this.#promises.push(resolve);
				this.doIdle();
			});
		});
	}
}


Object.defineProperty(globalThis, "screen", {
	enumerable: true,
	configurable: true,
	get() {		// instantiate screen on first access
		let value, width, height;
		
		if (config.Screen) { 
			value = new config.Screen({});
			width = value.width;
			height = value.height;
		}
		Object.defineProperty(globalThis, "screen", {
			enumerable: false,
			configurable: true,
			writable: true,
			value
		});

		const mc_width = (undefined === config.mc_width) ? undefined : Number(config.mc_width);
		const mc_height = (undefined === config.mc_height) ? undefined : Number(config.mc_height);
		screen = new Screen({width: mc_width ?? width ?? 240, height: mc_height ?? height ?? 320});
		screen.configure({show: ((undefined === mc_width) && (undefined === mc_height)) || ((width === mc_width) && (height === mc_height))});

		return screen;
	},
	set(value) {	// simulator creates screen
		Object.defineProperty(globalThis, "screen", {
			enumerable: false,
			configurable: true,
			writable: true,
			value
		});
	
		const mc_width = (undefined === config.mc_width) ? undefined : Number(config.mc_width);
		const mc_height = (undefined === config.mc_height) ? undefined : Number(config.mc_height);
		screen = new Screen({width: mc_width ?? value.width, height: mc_height ?? value.height});
	}
});

globalThis.$NETWORK = {
	get connected() {
		const WiFi = Modules.importNow("embedded:network/interface/wifi");

		assert(!!config.ssid, "Wi-Fi SSID missing");
		return new Promise((resolve, reject) => {
			const w = new WiFi({
				onChanged(property) {
					if (this.address) {
						Timer.clear(this.timer);
						this.close();
						resolve();
					}
				}
			});
			if (w.address) {
				resolve();
				return void w.close();
			}

			trace(`Connecting to Wi-Fi SSID "${config.ssid}"...\n`);
			w.connect(config.password ? {SSID: config.ssid, password: config.password, secure: true} : {SSID: config.ssid});
			w.timer = Timer.set(() => {
				w.close();
				$DONE("Wi-Fi connection attempt timed out");
			}, $TESTMC.wifiConnectionTimeout);
		});
	},
	async wifi(options) {
		// could be async to allow time to bring up an AP
		return {ssid: config.ssid, password: config.password};
	},
	async resolve(domain) {
		return new Promise((resolve, reject) => {
			const Net = Modules.importNow("net");
			Net.resolve(domain, (name, address) => {
				if (address)
					resolve(address);
				else
					reject();
			});
		});
	},
	invalidDomain: "fail.moddable.com",
};
Object.freeze(globalThis.$NETWORK);

class HostObject extends Native("xs_hostobject_destructor") {
	constructor() { super(); native("xs_hostobject").call(this); }
}

class HostObjectChunk extends Native("xs_hostobjectchunk_destructor") {
	constructor() { super(); native("xs_hostobjectchunk").call(this); }
}

class HostBuffer extends Native("xs_hostbuffer_destructor") {
	constructor() { super(); native("xs_hostbuffer").call(this); }
}

class TestBehavior extends (globalThis.Behavior ?? Object) {
	constructor() {
		super();

		for (const name of Object.getOwnPropertyNames(Object.getPrototypeOf(this))) {
			if (!name.startsWith("on")) continue;
			const value = this[name];
			Object.defineProperty(this, name, {
				value: (content, ...args) => {try {return value.call(this, content, ...args);} catch (e) {$DONE(e)}}
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
	wifiInvalidConnectionTimeout: 30_000,
	wifiConnectionTimeout: 20_000,
	wifiScanTimeout: 10_000,
	Behavior: TestBehavior
};

Object.freeze(globalThis.$TESTMC, true);

export default function() {
	const former = globalThis.assert;
	function assert(...args) {
		former(...args);
	}
	Object.setPrototypeOf(assert, former);
	globalThis.assert = assert;
	$MAIN();
}
