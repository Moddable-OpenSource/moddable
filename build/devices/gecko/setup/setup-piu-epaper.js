/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import DESTM32S from "destm32s";
import Timer from "timer";
import Time from "time";

class Screen extends DESTM32S {
	start(interval) {
		this.timer = Timer.repeat(() => {
			this.context.onIdle();
		}, interval);

	}
	stop() {
		Timer.clear(this.timer);
		delete this.timer;

	}
	clear() {
	}
}

export default function () {
	global.screen = new Screen({width: 122, height: 250});
	Timer.set(main);
}

function main() {
	let f = require.weak("main");
	if (typeof f === "function")
		f.call(this);
}
