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
/*
 *     To set the device clock on launch, configure the device's Wi-Fi network from the command line:
 *     mcconfig -d -m -p <platform> ssid=<your ssid> password=<your psk>
 */

import Timer from "timer";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen);
let backgroundColor = render.makeColor(0, 0, 255);
let digitsColor = render.makeColor(255, 255, 255);
let digits = parseBMP(new Resource("digits-alpha.bmp"));

const digitWidth = (digits.width / 10);
const digitHeight = digits.height;
const colonWidth = 16;
let colon = 1;
let timeWidth = (digitWidth << 2) + colonWidth;
let bounds = { x:(render.width - timeWidth) >> 1, y:(render.height - digitHeight) >> 1, width:timeWidth, height:digitHeight };

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
render.end();

Timer.repeat(id => {
	render.begin(bounds.x, bounds.y, bounds.width, bounds.height);
		let d = new Date();
		let h = d.getHours();
		let m = d.getMinutes();
		let x = bounds.x;
		let y = bounds.y;
		render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
		render.drawGray(digits, digitsColor, x, y, Math.floor(h / 10) * digitWidth, 0, digitWidth, digitHeight);
		x += digitWidth;
		render.drawGray(digits, digitsColor, x, y, (h % 10) * digitWidth, 0, digitWidth, digitHeight);
		x += digitWidth;
		if (colon) {
			render.fillRectangle(digitsColor, x + 6, y + 10, 6, 6);
			render.fillRectangle(digitsColor, x + 6, y + digitHeight - 18, 6, 6);
		}
		x += colonWidth;
		render.drawGray(digits, digitsColor, x, y, Math.floor(m / 10) * digitWidth, 0, digitWidth, digitHeight);
		x += digitWidth;
		render.drawGray(digits, digitsColor, x, y, (m % 10) * digitWidth, 0, digitWidth, digitHeight);
		colon = !colon;
	render.end();
}, 500);

