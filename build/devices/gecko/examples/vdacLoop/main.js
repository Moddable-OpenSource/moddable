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

import Timer from "timer";
import Vdac from "pins/vdac";

import DESTM32S from "destm32s";
import Poco from "commodetto/Poco";
import parseRLE from "commodetto/parseRLE";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";
import Sleep from "sleep";

import Analog from "pins/analog";
import Digital from "pins/digital";

// sweeps voltage from 0 to 2.5v on vdac pin

let max_count = 4095;
let VDAC_STEP = 50;

//** drawing bits
const screenWidth = 250;
const screenHeight = 122;

export default function() {
	global.dir = VDAC_STEP;

	let flip = 0;
	let count = 0;

	global.poco = new Poco(new DESTM32S({width: screenHeight, height: screenWidth, clear: false}), {rotation:90, displayListLength:8192});

	global.black = poco.makeColor(0, 0, 0);
	global.white = poco.makeColor(255, 255, 255);
	global.openSans18 = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));

	function display(value, voltage, measured) {
		let x, y;
		let text;
		poco.begin(0, 0, screenWidth, screenHeight);

		poco.fillRectangle(flip%1 ? white : black, 0, 0, screenWidth, screenHeight);
		x = 2;
		y = 1;
		text = "value: " + value;
		poco.drawText(text, openSans18, flip%1 ? black : white, x, y);
		y += 25;
	
		text = " voltage: " + voltage.toFixed(3);
		poco.drawText(text, openSans18, flip%1 ? black : white, x, y);
		y += 25;

		text = " measured: " + measured.toFixed(3) + "\n";
		poco.drawText(text, openSans18, flip%1 ? black : white, x, y);

		poco.end();
	}

	Timer.repeat(() => {
		let expected = 0;

		expected = (count * 2.5) / max_count;

		Vdac.write(1, count);
		Timer.delay(100);
		let measured = Analog.read(2);

		let calcd = (2.5 * measured) / 65536.0;

		display(count, expected, calcd);

		if (count > max_count)
			global.dir = -50;
		if (count < 0)
			global.dir = 50;

		count += dir;
		flip += 1;
	}, 100);

}
