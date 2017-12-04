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

import config from "mc/config";
import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(global.screen ? screen : new (require(config.screen))({}));

let backgroundColor = render.makeColor(0, 0, 0);

let sprite = parseBMP(new Resource("spinner-strip-80px-24cell-blue-color.bmp"));
sprite.alpha = parseBMP(new Resource("spinner-strip-80px-24cell-blue-alpha.bmp"));

let spriteWidth = sprite.width / 24;
let spriteHeight = sprite.height;
let spriteCellIndex = 0;

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
render.end();

Timer.repeat(id => {
	let x = (render.width - spriteWidth) >> 1;
	let y = (render.height - spriteHeight) >> 1;
	let sx = spriteCellIndex * spriteWidth;
	let sy = 0;
	render.begin(x, y, spriteWidth, spriteHeight);
		render.fillRectangle(backgroundColor, x, y, spriteWidth, spriteHeight);
		render.drawMasked(sprite, x, y, sx, sy, spriteWidth, spriteHeight, sprite.alpha, sx, sy);
	render.end();
	
	if (++spriteCellIndex > 23)
		spriteCellIndex = 0;
}, 33);

