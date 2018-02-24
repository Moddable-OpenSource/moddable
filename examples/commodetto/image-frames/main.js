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
import Poco from "commodetto/Poco";
import Stream from "commodetto/readStream";
import Resource from "Resource";

let render = new Poco(screen);
let backgroundColor = render.makeColor(255, 255, 255);

let buffer = new Resource("cell-flag.cs");
let stream = new Stream(buffer, { loop:true });
let x = (render.width - stream.width) >> 1, y = (render.height - stream.height) >> 1;

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
render.end();

Timer.repeat(id => {
	render.begin(x, y, stream.width, stream.height);
		render.drawFrame(stream.next(), stream, x, y);
	render.end();
}, 33);
