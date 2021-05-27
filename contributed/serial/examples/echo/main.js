/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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
import Serial from "serial";

let serial = new Serial();

/* 10 second timeout for reading strings */
serial.setTimeout(10000);

export default function() {
	Timer.repeat(() => {
		let str = serial.readLine();
		while (undefined !== str) {
			trace("got: " + str + "\n");
			serial.writeLine("[" + str + "]");
			str = serial.readLine();
		}
	}, 1000);
}
