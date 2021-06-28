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

import Poco from "commodetto/Poco";

let render = new Poco(screen);

let black = render.makeColor(0, 0, 0);
let blue = render.makeColor(0, 0, 255);
let green = render.makeColor(0, 255, 0);
let red = render.makeColor(255, 0, 0);
let gray = render.makeColor(128, 128, 128);

render.begin();
	render.fillRectangle(gray, 0, 0, render.width, render.height);

	render.origin(20, 20);
	render.fillRectangle(red, 0, 0, 80, 40);

	render.origin(65, 65);
	render.fillRectangle(green, 0, 0, 80, 40);

	render.origin(65, 65);
	render.fillRectangle(blue, 0, 0, 80, 40);

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();
render.end();
