/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
import Bitmap from "commodetto/Bitmap"
import Timer from "timer";

if (!config.Screen && !device.display?.default)
	throw new Error("no screen configured");

class Screen extends (config.Screen ?? device.display.default.io) {
	#timer;

	constructor(options) {
		super(options);

		this.#timer = Timer.set(() => {
			this.context.onIdle();
		}, 1, 100);
		Timer.schedule(this.#timer);
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
	globalThis.screen = new Screen(config.Screen ? {} : device.display.default);		// may overwrite Commodetto screen. that's oK.
	screen.pixelFormat ??= Bitmap[config.format];
	screen.configure({format: screen.pixelFormat});

	done();
}
