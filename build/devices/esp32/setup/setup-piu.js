/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import Time from "time";
import WiFi from "wifi";
import Net from "net";
import SNTP from "sntp";

class Screen extends require(config.screen) {
	start(interval) {
		this.timer = Timer.repeat(() => {
			let touch = this.touch;
			if (touch) {
				let points = touch.points;
				touch.read(points);
				for (let i = 0; i < points.length; i++) {
				  let point = points[i];
				  switch (point.state) {
					  case 0:
					  case 3: if (point.down) {
								  delete point.down;
								  this.context.onTouchEnded(i, point.x, point.y, Time.ticks);
								  delete point.x;
								  delete point.y;
							  }
							  break;
					  case 1: if (!point.down) {
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

		if (config.touch) {
			let touch = require(config.touch);
			touch = new touch;
			touch.points = [];
			let touchCount = config.touchCount ? config.touchCount : 1;
			while (touchCount--)
				touch.points.push({});
			this.touch = touch;
		}
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

export default function () {
	global.screen = new Screen({});

	WiFi.mode = 1;

	if (!config.ssid)
		return Timer.set(main);

	let monitor = new WiFi({ssid: config.ssid, password: config.password}, msg => {
		if ("gotIP" == msg) {
			trace(`IP address ${Net.get("IP")}\n`);

			monitor = undefined;
			if (!config.sntp)
				return Timer.set(main);

			Net.resolve(config.sntp, (name, address) => {
				if (!address)
					return trace("unable to resolve sntp host\n");

				trace(`resolved ${name} to ${address}\n`);

				new SNTP({address}, function(message, value) {
					if (1 == message) {
						Time.set(value);
						trace("got time\n");
					}
					else if (-1 == message)
						trace("unable to retrieve time\n");
					else
						return;
					Timer.set(main);
				});
			});
		}
		else if ("connect" == msg)
			trace(`Wi-Fi connected to "${Net.get("SSID")}"\n`);
		else if ("disconnect" == msg)
			trace("Wi-Fi disconnected\n");
	});
}

function main() {
	let f = require.weak("main");
	if (typeof f === "function")
		f.call(this);
}

