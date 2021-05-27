/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Timer from "timer";
import DotStar from "dotstar";
import Poco from "commodetto/Poco";
import parseBMP from "commodetto/parseBMP";
import Resource from "Resource";

let lights = parseBMP(new Resource("lights-color.bmp"));

const width = lights.width, height = 1;
let display = new DotStar({width, height});
let render = new Poco(display);

let y = 0;
Timer.repeat(() => {
	y += 1;
	if ((y + height) > lights.height)
		y = 0;

	render.begin(0, 0, width, height);
		render.drawBitmap(lights, 0, 0, 0, y, width, height);
	render.end();
}, 20);
