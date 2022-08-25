/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Preference from "preference";

function drawTime(render, when) {
	let backgroundColor = render.makeColor(255, 255, 255);
	let digitsColor = render.makeColor(0, 0, 0);
	let colonColor = render.makeColor(128, 128, 128);
	let digits = parseBMP(new Resource("digits-alpha.bmp"));

	const digitWidth = Math.idiv(digits.width, 10);
	const digitHeight = digits.height;
	const colonWidth = 16;
	const timeWidth = (digitWidth << 2) + colonWidth;
	const bounds = { x:(render.width - timeWidth) >> 1, y:(render.height - digitHeight) >> 1, width:timeWidth, height:digitHeight };

	render.begin();
		render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);

		let h = when.getHours();
		let m = when.getMinutes();
		let x = bounds.x;
		let y = bounds.y;
		render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
		if (Math.idiv(h, 10)) {
			render.drawGray(digits, digitsColor, x, y, Math.idiv(h, 10) * digitWidth, 0, digitWidth, digitHeight);
			x += digitWidth;
		}
		else
			x += digitWidth >> 1;
		render.drawGray(digits, digitsColor, x, y, (h % 10) * digitWidth, 0, digitWidth, digitHeight);
		x += digitWidth;

		render.fillRectangle(colonColor, x + 6, y + 10, 6, 6);
		render.fillRectangle(colonColor, x + 6, y + digitHeight - 18, 6, 6);
		x += colonWidth;
		render.drawGray(digits, digitsColor, x, y, Math.idiv(m, 10) * digitWidth, 0, digitWidth, digitHeight);
		x += digitWidth;
		render.drawGray(digits, digitsColor, x, y, (m % 10) * digitWidth, 0, digitWidth, digitHeight);
	render.end();
}

export default function () {
	const rtc = new device.peripheral.RTC;
	if (rtc.time !== undefined && (Date.now() > (new Date("July 1, 2021")).valueOf()))
		rtc.time = Date.now();

	rtc.configure({alarm: rtc.time + 60_000});	// 1 minute

	const led = new device.peripheral.led.Default;
	led.on = 1;

	Time.set(rtc.time / 1000);

	const battery = new device.peripheral.battery.Default;
	trace(`Battery level ${battery.read()}\n`);
	battery.close();

	const render = new Poco(screen);
	
	const now = new Date;
	const previous = new Date(parseFloat(Preference.get("clock", "time")) ?? 0);
	const n = Math.floor(now / 60_000), p = Math.floor(previous / 60_000)
	if (n === (p + 1)) {		// one minute difference, use partial update
		screen.configure({previous: true, refresh: false});
		drawTime(render, previous);
	}
	drawTime(render, now);

	Preference.set("clock", "time", now.valueOf().toString());

	screen.close();
	led.on = 0;

	power.main.write(0);

	Timer.repeat(() => {
		led.on = 1 - led.on;
	}, 200);
}
