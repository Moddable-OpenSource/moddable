/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import config from "mc/config";
import Time from "time";
import Timer from "timer";
import Button from "pebble/button";

if (!config.Screen)
	throw new Error("no screen configured");

function resize(progress) {
trace(`piu setup resize\n`);
	screen.context.onResize(progress);
}

class Screen extends config.Screen {
	#timer;
	#touch;
	#button;

	constructor(options) {
		super(options);
	}
	get context() {
		if (this.#touch)
			return this.#touch?.context;
	}
	set context(it) {
		if (!it) {
			this.#touch?.close();
			this.#touch = undefined;

			this.#button?.close(); 
			this.#button = undefined; 

			Timer.clear(this.#timer);
			this.#timer = undefined; 

			watch.removeEventListener("resize", resize);
			return;
		}

		if (this.#touch) {
			this.#touch.context = context;
			return;
		}

		this.#timer = Timer.set(() => {
			this.#touch.context.onIdle();
		}, 1, 100);
		Timer.schedule(this.#timer);

		this.#button = new Button({
			types: ["select", "up", "down", "back"],
			onPush: (state, which) => {
// 				trace(`${state ? "press" : "release"} ${which}\n`);
				it.onButton(state, which);
			}
		});
		watch.addEventListener("resize", resize);

		let touchCount = config.touchCount ?? 1;
		const Touch = config.Touch || globalThis.device?.sensor?.Touch;
		if (!Touch || !touchCount) {
			this.#touch = {context: it};
			return;
		}

		const onSample = () => {
			const touch = this.#touch;
			const points = touch.sample();
			if (!points) return;

			let mask = (1 << touchCount) - 1;
			for (let i = 0, length = points.length; i < length; i++) {
				const point = points[i];
				const id = point.id;
				const last = touch.points[id];

				mask ^= 1 << id;
				this.rotate?.(point);
				if (last) {
					last.x = point.x;
					last.y = point.y;
					touch.context.onTouchMoved(id, point.x, point.y, Time.ticks);
				}
				else {
					touch.points[id] = {x: point.x, y: point.y};
					touch.context.onTouchBegan(id, point.x, point.y, Time.ticks);
				}
			}

			for (let i = 0; mask; i += 1, mask >>= 1) {
				if (mask & 1) {
					const last = touch.points[i];
					if (last) {
						touch.points[i] = undefined;
						touch.context.onTouchEnded(i, last.x, last.y, Time.ticks);
					}
				}
			}
		};

		const touch = new Touch({onSample});
		this.#touch = touch;
		this.#touch.context = it;

		touch.points = new Array(touchCount);
	}
	get rotation() {
		return 0;
	}
	
	clear() {
	}
	start(interval) {
		if (!this.#timer)
			return;

		if (interval < 17)
			interval = 17;

		Timer.schedule(this.#timer, interval, interval);
	}
	stop() {
		if (!this.#timer)
			return;

		Timer.schedule(this.#timer);
	}
}

export default function (done) {
	globalThis.screen = new Screen({});		// may overwrite Commodetto screen. that's OK.

	done();
}
