/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
import parseNFNT from "commodetto/parseNFNT";

let fonts = [
	"chicago12",
	"sanfrancisco",
].map(name => parseNFNT(new Resource(name + ".dat")));

let render = new Poco(screen);
let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);

let index = 0;
Timer.set(() => {
	let font = fonts[index++ % fonts.length];
	render.begin();
		render.fillRectangle(white, 0, 0, render.width, render.height);
		render.drawText(String.fromCharCode(0xF8FF) + " File Edit Goodies Font", font, black, 10, 10);
		render.drawText("Hello, again.", font, black, 10, 10 + font.height + font.leading);
	render.end();
}, 0, 250);
