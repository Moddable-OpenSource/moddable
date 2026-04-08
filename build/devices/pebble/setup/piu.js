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
import Timer from "timer";
import Button from "pebble/button";

if (!config.Screen)
	throw new Error("no screen configured");

function resize(progress) {
	screen.context.onResize(progress);
}

class Screen extends config.Screen {
	#context;
	#timer;
	#button;

	constructor(options) {
		super(options);
	}
	get context() {
		return this.#context;
	}
	set context(it) {
		this.#context = it;
		if (it) {
			this.#timer = Timer.set(() => {
				it.onIdle();
			}, 1, 100);
			Timer.schedule(this.#timer);
			this.#button = new Button({
				types: ["select", "up", "down", "back"],
				onPush: (state, which) => {
// 					trace(`${state ? "press" : "release"} ${which}\n`);
					it.onButton(state, which);
				}
			});
			watch.addEventListener("resize", resize);
		}
		else {
			this.#button?.close(); 
			this.#button = undefined; 

			Timer.clear(this.#timer);
			this.#timer = undefined; 

			watch.removeEventListener("resize", resize);
		}
	}
	get rotation() {
		return 0;
	}
	
	clear() {
	}
	start(interval) {
		if (interval < 17)
			interval = 17;

		Timer.schedule(this.#timer, interval, interval);
	}
	stop() {
		Timer.schedule(this.#timer);
	}
}

export default function (done) {
	globalThis.screen = new Screen({});		// may overwrite Commodetto screen. that's OK.

	done();
}
