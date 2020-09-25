/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

if (!config.Screen)
	throw new Error("no screen configured");

class Screen extends config.Screen {
	#timer = Timer.set(() => {
		this.context.onIdle();
	}, 0x7FFF, 0x7FFF);

	start(interval) {
		if (interval < 5)
			interval = 5;
		if (this.#timer.interval === interval)
			return;
		Timer.schedule(this.#timer, interval, interval);
		this.#timer.interval = interval;
	}
	stop() {
		Timer.schedule(this.#timer, 0x7FFF, 0x7FFF);
		delete this.#timer.interval;
	}
	clear() {
	}
}

export default function (done) {
	globalThis.screen = new Screen({});
	done();
}
