/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

import BM8563 from "bm8563";
import Time from "time";


let rtc = new BM8563;
let enabled = 1;

// Main button:  enable/disable RTC
button.a.onChanged = function () {
	if (button.a.read()) {
		return;
	}
	enabled = !enabled;
	rtc.enabled = enabled;
	global.power.brightness = 20 + enabled * 70;
}

// Side button: set time from sntp
button.b.onChanged = function () {
	if (button.b.read()) {
		return;
	}
	setRTCTimeLocal();
}

import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import config from "mc/config";

function setRTCTimeLocal() {
	let d = new Date();
	trace(`Set time: ${d.getTime()} ${d.toString()} tz:${Time.timezone} dst:${Time.dst}\n`);
	rtc.seconds = d.getTime() / 1000;
	let read = rtc.seconds;
	let s = new Date(read * 1000);
	trace(`Get time: ${read} ${s.toString()}\n`)
}

const render = new Poco(screen, {
	rotation: config.rotation
});

let white = render.makeColor(255, 255, 255);
let grey = render.makeColor(170, 170, 170);
let blue = render.makeColor(0, 0, 255);

let font = parseBMF(new Resource("OpenSans-Semibold-16.bf4"));
let text = "Press button A to set Time";
let textWidth = render.getTextWidth(text, font);
let x = 1;
let y = 1;

render.begin();
render.fillRectangle(blue, 0, 0, render.width, render.height);
render.end();

Timer.repeat(id => {
	let now = 'setting..';
	try {
		now = rtc.seconds;
	} catch (e) {
		trace(e);
	}
	let rtc_clock = new Date(now * 1000);
	let actual_clock = new Date();
	render.begin(0, y, render.width, render.height);
	render.fillRectangle(blue, 0, 0, render.width, render.height);
	render.drawText(rtc_clock.toString().slice(4, 24), font, white, x, y);
	render.drawText(actual_clock.toString().slice(4, 24), font, grey, x, y + 30);
	render.end();
}, 1000);