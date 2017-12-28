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

import Poco from "commodetto/Poco";
import Timer from "timer";

let poco = new Poco(screen);

let gray = 0, step = 3;
Timer.repeat(() => {
	poco.begin();
		poco.fillRectangle(poco.makeColor(255, 255, 255), 0, 0, poco.width, poco.height);
		poco.fillRectangle(poco.makeColor(gray, gray, gray), 4, 4, poco.width - 8, poco.height - 8);
	poco.end();

	gray += step;
	if (gray >= 255) {
		gray = 255;
		step = -step;
	}
	else if (gray <= 0) {
		gray = 0;
		step = -step;
	}
}, 16);
