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

/* Demonstrates the use of the Sharp mirror display on the */
/* SiLabs gecko device, deep sleeping between inversions of */
/* the Moddable logo on the display. */

import LS013B4DN04 from "ls013b4dn04";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import parseBMP from "commodetto/parseBMP";
import Sleep from "sleep";
import Timer from "timer";

let SLEEP_MS = 5000;

export default function() {
	let logo = parseBMP(new Resource("moddable-white.bmp"));
	let sleep = new Sleep();

	const width = 96, height = 96;
	let render = new Poco(new LS013B4DN04({width: width, height: height}));

	let black = render.makeColor(0, 0, 0);
	let white = render.makeColor(255, 255, 255);

	let index = 0;

	if (Sleep.getWakeupCause() != 3)
		index = Sleep.getPersistentValue(0) ? 1 : 0;

//let timer = Timer.repeat(() => {
	index ^= 1;
	render.begin(0, 0, width, height);
		render.fillRectangle(index ? white : black, 0, 0, width, height);
		render.drawGray(logo, index ? black : white, 0, 30);
	render.end();
//}, SLEEP_MS);

	Sleep.setPersistentValue(0, index);
	Sleep.sleepEM4(SLEEP_MS);
}

