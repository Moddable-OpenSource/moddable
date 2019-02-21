/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import ILI9341 from "ili9341";
import Timer from "timer";
import Time from "time";
import Touch from "xpt2046";

class Screen extends ILI9341 {
	start(interval) {
		this.timer = Timer.repeat(() => {
			let touch = this.touch;
			let point = touch.read(touch.point);
			if (point) {
				if (touch.down)
					this.context.onTouchMoved(0, point.x, point.y, Time.ticks);
				else {
					touch.down = true;
					this.context.onTouchBegan(0, point.x, point.y, Time.ticks);
				}
			}
			else if (touch.down) {
				delete touch.down;
				let point = touch.point;
				this.context.onTouchEnded(0, point.x, point.y, Time.ticks);
			}
			this.context.onIdle();
		}, interval);

		let touch = new Touch();
		touch.begin(this.width, this.height);
		touch.point = {};
		this.touch = touch;
	}
	stop() {
		Timer.clear(this.timer);
		delete this.timer;

		this.touch.end();
		delete this.touch;
	}
	clear() {
	}
}

export default function () {
	global.screen = new Screen({width: 240, height: 320});
	Timer.set(main);
}

function main() {
	let f = require.weak("main");
	if (typeof f === "function")
		f.call(this);
}

