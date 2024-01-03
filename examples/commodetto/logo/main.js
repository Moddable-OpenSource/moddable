/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";

const render = new Poco(screen);
const logo = parseBMP(new Resource("logo-color.bmp"));
render.begin();
	render.fillRectangle(render.makeColor(255, 255, 255),0, 0, render.width, render.height);
	render.drawBitmap(logo, (render.width - logo.width) / 2, (render.height - logo.height) / 2);
render.end();
