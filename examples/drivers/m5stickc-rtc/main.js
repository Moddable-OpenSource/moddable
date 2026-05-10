/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

import Time from "time";
import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import config from "mc/config";

const rtc = new device.peripheral.RTC();

// Main button: set time from sntp
globalThis.button.a.onChanged = () => {
if (globalThis.button.a.read()) {
		return;
	}
	setRTCTimeLocal();
}

function setRTCTimeLocal() {
	const d = new Date();
	trace(`Set time: ${d.getTime()} ${d.toString()} tz:${Time.timezone} dst:${Time.dst}\n`);
	rtc.time = d.getTime();
	const read = rtc.time;
	const s = new Date(read);
	trace(`Get time: ${read} ${s.toString()}\n`)
}

const render = new Poco(screen, {
	rotation: config.rotation
});

const white = render.makeColor(255, 255, 255);
const grey = render.makeColor(170, 170, 170);
const blue = render.makeColor(0, 0, 255);

const font = parseBMF(new Resource("OpenSans-Semibold-16.bf4"));
const x = 1;
const y = 1;

render.begin();
render.fillRectangle(blue, 0, 0, render.width, render.height);
render.end();

Timer.repeat(_id => {
	let now = 'setting..';
	try {
		now = rtc.time;
	} catch (e) {
		trace(e);
	}
	const rtc_clock = new Date(now);
	const actual_clock = new Date();
	render.begin(0, y, render.width, render.height);
	render.fillRectangle(blue, 0, 0, render.width, render.height);
	render.drawText(rtc_clock.toString().slice(4, 24), font, white, x, y);
	render.drawText(actual_clock.toString().slice(4, 24), font, grey, x, y + 30);
	render.end();
}, 1000);