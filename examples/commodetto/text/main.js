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

import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen, { displayListLength: 2048 });

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let blue = render.makeColor(0, 0, 255);
let green = render.makeColor(0, 255, 0);
let red = render.makeColor(255, 0, 0);
let yellow = render.makeColor(255, 255, 0);

let font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
let text;

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Left", font, blue, 2, 2);
	render.drawText("Right", font, green, render.width - render.getTextWidth("Right", font) - 2, 2);
	render.drawText("Center", font, white,
    	(render.width - render.getTextWidth("Center", font)) >> 1,
    	(render.height - font.height) >> 1);
    	
    text = "Clip text that is wider than the screen";
	render.drawText(text, font, red, 2, render.height - font.height - 2);

    text = "Truncate text that is wider than the screen";
	render.drawText(text, font, yellow, 2, render.height - (font.height << 2), render.width - 2);
render.end();
