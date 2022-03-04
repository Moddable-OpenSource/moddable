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

import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

const font = new Resource("8x8font.dat");

const render = new Poco(screen, {displayListLength: 1536});
const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);

const colors = [
	render.makeColor(255, 255, 255),
	render.makeColor(0, 255, 0),
	render.makeColor(255, 0, 0),
	render.makeColor(0, 0, 255),
	render.makeColor(255, 255, 0),
	render.makeColor(0, 255, 255),
	render.makeColor(255, 0, 255),
];

const text = [
	"Moddable SDK",
	"JavaScript",
	"8x8 font",
	"cfe8x8 example"
];

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
render.end();

let color = 0;

Timer.repeat(() => {
	render.begin(0, 0, render.width, render.height);
	render.fillRectangle(white, 0, 0, render.width, render.height);
	render.fillRectangle(black, 2, 2, render.width - 4, render.height - 4);

	const lineHeight = 9, lines = text.length;
	for (let i = 0, y = (render.height - lines * lineHeight) >> 1; i < lines; i++, y += lineHeight)
		render.drawText(text[i], font, colors[color % colors.length], (render.width - render.getTextWidth(text[i], font)) >> 1, y);
	render.end();
	color += 1;
}, 1000 / 12);
