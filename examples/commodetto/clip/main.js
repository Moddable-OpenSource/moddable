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

let render = new Poco(screen);

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let green = render.makeColor(0, 255, 0);
let red = render.makeColor(255, 0, 0);
let yellow = render.makeColor(255, 255, 0);
let font = parseBMF(new Resource("OpenSans-Semibold-28.bf4"));
let text = "Clipped!";

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	
	render.clip(20, 20, render.width - 40, render.height - 40);
	render.fillRectangle(white, 0, 0, render.width, render.height);
	
	render.clip(40, 40, render.width >> 1, render.height - 80);
	render.fillRectangle(green, 0, 0, render.width, render.height);
	render.drawText(text, font, black, (render.width - render.getTextWidth(text, font)) >> 1, (render.height - font.height) >> 1);
	render.clip();
	
	render.clip(render.width >> 1, 40, (render.width >> 1) - 40, render.height - 80);
	render.fillRectangle(red, 0, 0, render.width, render.height);
	render.drawText(text, font, yellow, (render.width - render.getTextWidth(text, font)) >> 1, (render.height - font.height) >> 1);
	render.clip();
	
	render.clip();
render.end();
