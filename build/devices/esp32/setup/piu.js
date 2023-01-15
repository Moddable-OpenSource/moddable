/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

if (!config.Screen)
	throw new Error("no screen configured");

class Screen extends config.Screen {
	#timer;
	#touch;

	set context(value) {
		if (!value) {
			if (this.#touch) {
				Timer.clear(this.#touch.timer);
				this.#touch?.close();
				this.#touch = undefined; 
			}
			if (this.#timer) {
				Timer.clear(this.#timer);
				this.#timer = undefined; 
			}
			return;
		}
		if (this.#touch) {
			this.#touch.context = context;
			return;
		}		

		// build timer
		this.#timer = Timer.set(() => {
			this.#touch.context.onIdle();
		}, 1, 100);
		Timer.schedule(this.#timer);

		// build touch instance
		let touchCount = config.touchCount ?? 1;
		const Touch = config.Touch || globalThis.device?.sensor?.Touch;
		if (!Touch || !touchCount) {
			this.#touch = {context: value};
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
		this.#touch.context = value;

		if (touch.sample) {	// ECMA-419 driver
			touch.points = new Array(touchCount);
			if (!touch.configuration?.interrupt)
				touch.timer = Timer.repeat(onSample, 16);
		}
		else {		// legacy driver
			touch.points = [];
			while (touchCount--)
				touch.points.push({});

			touch.timer = Timer.repeat(() => {
				const touch = this.#touch;
				let points = touch.points;
				touch.read(points);
				for (let i = 0, length = points.length; i < length; i++) {
					const point = points[i];
					this.rotate?.(point);
					switch (point.state) {
						case 0:
						case 3:
							if (point.down) {
								delete point.down;
								touch.context.onTouchEnded(i, point.x, point.y, Time.ticks);
								delete point.x;
								delete point.y;
							}
							break;
						case 1:
						case 2:
							if (!point.down) {
								point.down = true;
								touch.context.onTouchBegan(i, point.x, point.y, Time.ticks);
							}
							else
								touch.context.onTouchMoved(i, point.x, point.y, Time.ticks);
						break;
					}
				}
			}, 16);
		}
	}
	get context() {
		return this.#touch?.context;
	}
	get rotation() {
		return super.rotation;
	}
	set rotation(value) {
		if (config.rotation)
			value = (value + config.rotation) % 360;
		super.rotation = value;
		this.rotate = rotate[value];
	}
	clear() {
	}
	start(interval) {
		const timer = this.#timer;
		if (!timer) return;

		if (interval < 5)
			interval = 5;

		Timer.schedule(timer, interval, interval);
	}
	stop() {
		const timer = this.#timer;
		if (!timer) return;

		Timer.schedule(timer);
	}
	animateColors(clut) {
		this.clut = clut;
		return true;		// need screen update
	}
}

const rotate = {
	90: function(point) {
			const x = point.x;
			point.x = point.y;
			point.y = this.height - x;
		},
	180: function(point) {
			point.x = this.width - point.x;
			point.y = this.height - point.y;
		},
	270: function(point) {
			const x = point.x;
			point.x = this.width - point.y;
			point.y = x;
		},
};
Object.freeze(rotate, true);

export default function (done) {
	globalThis.screen = new Screen({});
	if (config.driverRotation)
		screen.rotation = config.driverRotation;
	done();
}
