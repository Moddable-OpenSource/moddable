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
 *     This app should be built with rotation set to 90 degrees:
 *     mcconfig -d -m -p esp -r 90
 */

import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen, { rotation:90 });

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);

let image = parseBMP(new Resource("delmar-color.bmp"));
let font = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
let text = "Where the Turf Meets the Surf!";

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(image, 0, 0);
	render.drawText(text, font, white,
		(render.width - render.getTextWidth(text, font)) >> 1,
		render.height - ((render.height - image.width) >> 1) - (font.height >> 1));
render.end();
