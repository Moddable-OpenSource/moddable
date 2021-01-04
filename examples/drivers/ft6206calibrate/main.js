/*
 *     Copyright (C) 2016-2020 Moddable Tech, Inc.
 *     All rights reserved.
 */

import config from "mc/config";
import Timer from "timer";
import parseRLE from "commodetto/parseRLE";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Preference from "preference";

const touch = new config.Touch;
touch.points = [{}];

let touches = [];
let state = 0;
const locations = [{x: 10, y: 10}, {x: 190, y: 10}, {x: 190, y: 270}, {x: 10, y: 270}];
let grays = 255;
let graysDirection = -1;

const render = new Poco(new config.Screen({}));
const blue = render.makeColor(0, 0, 255);
const white = render.makeColor(255, 255, 255);
const crosshair = parseRLE(new Resource("crosshair-alpha.bm4"));
const success = parseRLE(new Resource("success-alpha.bm4"));

render.begin();
	render.fillRectangle(blue, 0, 0, render.width, render.height);
	render.drawGray(crosshair, white, locations[state].x, locations[state].y);
render.end();

Timer.repeat(id => {
	touch.read(touch.points);
	let where = touch.points[0];

	if (where.down && (0 === where.state))
		where.state = 3;

	if (0 == where.state) {
		render.begin(locations[state].x, locations[state].y, crosshair.width, crosshair.height);
			render.fillRectangle(blue, 0, 0, render.width, render.height);
			render.drawGray(crosshair, render.makeColor(grays, grays, grays), locations[state].x, locations[state].y);
			grays += graysDirection;
			if (grays >= 255) {
				grays = 255;
				graysDirection = -graysDirection;
			}
			else if (grays < 164) {
				grays = 164;
				graysDirection = -graysDirection;
			}
		render.end()
	}
	else if (1 === where.state) {
		 render.begin(locations[state].x, locations[state].y, crosshair.width, crosshair.height);
			 render.fillRectangle(blue, 0, 0, render.width, render.height);
			 render.drawGray(crosshair, white, locations[state].x, locations[state].y);
		 render.end();
		 where.down = true;
	}
	else if (3 === where.state) {
		delete where.down;
		touches.push({x: where.x, y: where.y})

		 let from = locations[state], to = ((state + 1) < locations.length) ? locations[state + 1] : {x: from.x, y: 320};
		 for (let i = 0; i <= 100; i++) {
			let x = ((to.x * i) + (from.x * (100 - i))) / 100;
			let y = ((to.y * i) + (from.y * (100 - i))) / 100;
			render.begin(x - 10, y - 10, crosshair.width + 20, crosshair.height + 20);
				render.fillRectangle(blue, 0, 0, render.width, render.height);
				render.drawGray(crosshair, white, x, y);
			render.end()
		 }

		grays = 255;
		state += 1;
	}

	if (state < locations.length)
		return;

	let flipX, flipY;
	if ((touches[0].x > touches[1].x) && (touches[3].x > touches[2].x))
		 flipX = true;
	else if ((touches[0].x < touches[1].x) && (touches[3].x < touches[2].x))
		flipX = false;

	 if ((touches[0].y > touches[2].y) && (touches[1].y > touches[2].y))
		 flipY = true;
	else if ((touches[0].y < touches[2].y) && (touches[1].y < touches[2].y))
		 flipY = false;

	if ((undefined === flipX) || (undefined === flipY)) {
		// confused. try again
		state = 0;
		touches = [];
		return;
	}

	let left = (touches[0].x + touches[3].x) / 2;
	let right = (touches[1].x + touches[2].x) / 2;
	let top = (touches[0].y + touches[1].y) / 2;
	let bottom = (touches[2].y + touches[3].y) / 2;

	let scale_x = (right - left) / 180;
	let x_min = Math.round(left - (30 * scale_x));
	let x_max = Math.round(right + (30 * scale_x));

	let scale_y = (bottom - top) / 260;
	let y_min = Math.round(top - (30 * scale_y));
	let y_max = Math.round(bottom + (30 * scale_y));

	Preference.set("ft6206", "calibrate", Int16Array.of(x_min, x_max, y_min, y_max).buffer);

	let x = (render.width - success.width) >> 1, y = (render.height - success.height) >> 1;
	for (let i = 0; i < 256; i += 2) {
		render.begin(x, y, success.width, success.height);
			render.fillRectangle(blue, 0, 0, render.width, render.height);
			render.drawGray(success, render.makeColor(i, i, 255), x, y);
		render.end();
	}

	 Timer.clear(id);
}, 10);
