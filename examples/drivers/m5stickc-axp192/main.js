/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc. , Satoshi Tanaka
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
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import config from "mc/config";

const render = new Poco(screen, {rotation:config?.rotation});

const white = render.makeColor(255, 255, 255);
const blue = render.makeColor(0, 0, 255);

const font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
const text = "Press button A to change brightness...   ";
const textWidth = render.getTextWidth(text, font);
let x = render.width;
let y = (render.height - font.height) >> 1;

const loop = true;	// set false to scroll text once across the screen
let brightness = 50;  // screen brightness %
globalThis.button.a.onChanged = () =>{
	if (globalThis.button.a.read()) {
		return;
	}
	brightness+=10;
	if (brightness > 100) {
		brightness = 0;
	}
	globalThis.power.brightness = brightness;
}

render.begin();
	render.fillRectangle(blue, 0, 0, render.width, render.height);
render.end();

Timer.repeat(id => {
	render.begin(0, y, render.width, font.height);
		render.fillRectangle(blue, 0, 0, render.width, render.height);
		render.drawText(text, font, white, x, y);
		if (!loop) {
			if (x + textWidth === 0)
				Timer.clear(id);
		}
		else {
			if (x + textWidth < render.width)
				render.drawText(text, font, white, x + textWidth, y);
		}
		if (x + textWidth === 0)
			x = 0;
		else
			--x;
	render.end();
}, 17);
