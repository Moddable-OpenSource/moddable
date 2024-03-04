/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Poco from "commodetto/Poco";
import Timer from "timer";

const render = new Poco(screen, {pixels: screen.width * 16});

render.begin();
	render.fillRectangle(render.makeColor(0, 0, 0), 0, 0, render.width, render.height);
render.end();

let step = 3;
let brightness = 0;
Timer.repeat(() => {
	render.begin(10, 10, render.width - 20, render.height - 20);
		render.fillStatic(0, 0, render.width, render.height, brightness);
		brightness += step;
		if (brightness > 255) {
			brightness = 255;
			step *= -1;
		}
		else if (brightness < 64) {
			brightness = 64;
			step *= -1;
		}
	render.end();
}, 33);
