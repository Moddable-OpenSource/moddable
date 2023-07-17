/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import {Sleep} from "sleep";
import Time from "time";
import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen);
let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let font = parseBMF(new Resource("OpenSans-Semibold-28.bf4"));

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	drawTime();
render.end();

let count = 0;
Timer.repeat(id => {
	render.begin();
		render.fillRectangle(black, 0, (render.height - font.height) >> 1, render.width, font.height);
		drawTime();
	render.end();
	if (++count == 20) {
		Timer.clear(id);
		Sleep.deep(5000);
	}
}, 500);

function getTimeString() {
	let date = new Date();
	let hours = String(date.getHours());
	let minutes = String(date.getMinutes());
	let seconds = String(date.getSeconds());
	if (1 == hours.length)
		hours = '0' + hours;
	if (1 == minutes.length)
		minutes = '0' + minutes;
	if (1 == seconds.length)
		seconds = '0' + seconds;
	return hours + ':' + minutes + ':' + seconds;
}

function drawTime() {
	let time = getTimeString();
	render.drawText(time, font, white,
		(render.width - render.getTextWidth(time, font)) >> 1,
		(render.height - font.height) >> 1);
}

