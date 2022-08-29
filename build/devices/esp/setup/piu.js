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

	constructor(dictionary) {
		super(dictionary);

		this.#timer = Timer.set(() => {
			this.context.onIdle();
		}, 1, 100);
		Timer.schedule(this.#timer);

		if (config.Touch) {
			let touchCount = config.touchCount ?? 1;
			if (!touchCount)
				return;

			const touch = new config.Touch;
			touch.points = [];
			while (touchCount--)
				touch.points.push({});

			Timer.repeat(() => {
				let points = touch.points;
				touch.read(points);
				for (let i = 0; i < points.length; i++) {
					let point = points[i];
					if (this.rotate)
						this.rotate(point);
					switch (point.state) {
						  case 0:
						  case 3:
								if (point.down) {
									delete point.down;
									this.context.onTouchEnded(i, point.x, point.y, Time.ticks);
									delete point.x;
									delete point.y;
								}
							  break;
						  case 1:
						  case 2:
								if (!point.down) {
									point.down = true;
									this.context.onTouchBegan(i, point.x, point.y, Time.ticks);
								}
								else
									this.context.onTouchMoved(i, point.x, point.y, Time.ticks);
								break;
					}
				}
			}, 16);
		}
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
		if (interval < 5)
			interval = 5;

		Timer.schedule(this.#timer, interval, interval);
	}
	stop() {
		Timer.schedule(this.#timer);
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
