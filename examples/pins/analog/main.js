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

import config from "mc/config";
import Analog from "pins/analog";
import Timer from "timer";
import Resource from "Resource";
import parseBMF from "commodetto/parseBMF";

import Poco from "commodetto/Poco";

let font = parseBMF(new Resource("myFont.bf4"));

let render = new Poco(screen);
let width = render.width, height = render.height;

let white = render.makeColor(255, 255, 255);
let black = render.makeColor(0, 0, 0);

Timer.repeat(id => {
	let value = 1024 - Analog.read(0);
	trace(value + "\n");

	value = value.toString();
	render.begin();
		render.fillRectangle(black, 0, 0, width, height);
		render.drawText(value, font, white, (width - render.getTextWidth(value, font)) >> 1, (height - font.ascent) >> 1);
	render.end();
}, 50);
