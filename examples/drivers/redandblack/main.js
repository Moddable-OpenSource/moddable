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
 *     This example should be built for the RGB332 pixel format and with rotation set to 90 degrees:
 *     mcconfig -d -m -r 90 -p esp -f rgb332
 */

import DESTM32S from "destm32s";
import Bitmap from "commodetto/Bitmap";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

let images = ["apple", "coke", "flowers", "spiderman", "stormtrooper"].map(name => parseBMP(new Resource(name + "-color.bmp")));
let index = 0;

let render = new Poco(new DESTM32S({clear: false}), {rotation: config.rotation});

let white = render.makeColor(255, 255, 255);
let black = render.makeColor(0, 0, 0);
let gray = render.makeColor(128, 128, 128);
let red = render.makeColor(255, 0, 0);

Timer.set(id => {
	render.begin();

	render.fillRectangle(red, 0, 0, render.width, render.height);
	
	// image width/height swapped in centering calculcation due to rotation
	let image = images[index];
	render.drawBitmap(image, (render.width - image.height) >> 1, (render.height - image.width) >> 1);

	render.end();

	index = (index + 1) % images.length;
}, 0, 20000);

