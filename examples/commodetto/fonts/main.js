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

import Poco from "commodetto/Poco";
import Resource from "Resource";
import parseBMF from "commodetto/parseBMF";
import config from "mc/config";
import Timer from "timer";

const poco = new Poco(screen, {displayListLength: 3096, pixels: 4096, rotation: config.rotation});

const black = poco.makeColor(0, 0, 0);
const white = poco.makeColor(255, 255, 255);

const margin = 8;

const fonts = [];
for (let font of Resource) {
	if (font.endsWith(".bf4"))
		fonts.push(font);
}
fonts.index = 0;

const texts = [
	"JavaScript is one of the worlds most widely used programming languages.",
	"JavaScript is one of the worlds most misunderstood languages.",
	"JavaScript is one of the worlds most loved & popular programming languages.",
	"JavaScript is one of the worlds most known scripting languages.",
	"JavaScript is one of the three core languages of Websites.",
	"JavaScript is one of the most popular modern web technologies!",
	"JavaScript is one of the fundamental coding languages of the web.",
	"JavaScript is one of the main building blocks of the web.",
	"JavaScript is one of the most dynamic scripting programming languages.",
	"JavaScript is one of the first languages many people learn.",
	"JavaScript is one of my working languages as is C++.",
	"JavaScript has been widely adopted for server and embedded applications."
];
texts.index = 0;

const textJP = "JavaScript （ジャバスクリプト) とは、プログラミング 言語 のひとつである。";

Timer.repeat(timer => {
	const index = fonts.index++ % fonts.length
	const name = fonts[index];
	const font = parseBMF(new Resource(name));
	let text = texts[texts.index++ % texts.length];
	if (name.includes("JP"))
		text = textJP;

	drawOne(font, text);
	
	if ((fonts.index / fonts.length) === 2)
		Timer.schedule(timer, 100, 600);
}, 100);

function drawOne(font, text) {
	poco.begin();
		poco.fillRectangle(white, 0, 0, poco.width, poco.height);

		text = text.split(" ");
		const layoutWidth = poco.width - (margin * 2); 
		let width = layoutWidth;
		let y = margin;
		const spaceWidth = poco.getTextWidth(" ", font);
		const lines = [""];
		while (text.length) {
			let wordWidth = poco.getTextWidth(text[0], font);
			if ((wordWidth < width) || (width === layoutWidth)) {
				if (lines[lines.length - 1])
					lines[lines.length - 1] += " ";
				lines[lines.length - 1] += text[0];
				text.shift();
			}
			width -= wordWidth + spaceWidth;
			if (width <= 0) {
				width = layoutWidth;
				y += font.height;
				lines.push("");
			}
		}
		if ("" === lines[lines.length - 1])
			lines.pop();

		for (let i = 0, y = (poco.height - (lines.length * font.height)) >> 1; i < lines.length; i++, y += font.height)
			poco.drawText(lines[i], font, black, (poco.width - poco.getTextWidth(lines[i], font)) >> 1, y);

	poco.end();
}
