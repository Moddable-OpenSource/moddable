/*
* Copyright (c) 2021  Moddable Tech, Inc.
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
import ReadGIF from "commodetto/ReadGIF";
import Timer from "timer";

const background = 0xFFFF;
const poco = new Poco(screen, {pixels: screen.width * 4});

poco.begin();
	poco.fillRectangle(background, 0, 0, poco.width, poco.height);
poco.end();

const reader = new ReadGIF(new Resource("cell-flag.gif"));

const x = (screen.width - reader.width) >> 1;
const y = (screen.height - reader.height) >> 1;

Timer.repeat(id => {
	reader.next();

	Timer.schedule(id, reader.frameDuration, reader.frameDuration);

	poco.begin(x + reader.frameX, y + reader.frameY, reader.frameWidth, reader.frameHeight);
		poco.fillRectangle(background, 0, 0, poco.width, poco.height);
		poco.drawBitmapWithKeyColor(reader, x, y);
	poco.end();
}, 1);
