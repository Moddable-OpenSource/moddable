/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	start(interval) {
		this.timer = Timer.repeat(() => {
			let touch = this.touch;
			if (touch) {
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
								if (!point.down) {
									point.down = true;
									this.context.onTouchBegan(i, point.x, point.y, Time.ticks);
								}
								break;
							case 2: this.context.onTouchMoved(i, point.x, point.y, Time.ticks);
								break;
					}
				}
			}
			this.context.onIdle();
		}, interval);

		if (config.Touch) {
			let touch = new config.Touch;
			touch.points = [];
			let touchCount = config.touchCount ? config.touchCount : 1;
			while (touchCount--)
				touch.points.push({});
			this.touch = touch;
		}
	}
	get rotation() {
		return super.rotation;
	}
	set rotation(value) {
		super.rotation = value;
		if (0 === value)
			delete this.rotate;
		else if (90 === value)
			this.rotate = function(point) {
				const x = point.x;
				point.x = point.y;
				point.y = this.height - x;
			};
		else if (180 === value)
			this.rotate = function(point) {
				point.x = this.width - point.x;
				point.y = this.height - point.y;
			};
		else if (270 === value)
			this.rotate = function(point) {
				const x = point.x;
				point.x = this.width - point.y;
				point.y = x;
			};
	}
	stop() {
		Timer.clear(this.timer);
		delete this.timer;

		if (this.touch)
			delete this.touch;
	}
	clear() {
	}
}

export default function (done) {
	global.screen = new Screen({});
	done();
}
