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

/* this is a simple demonstration of using serial.poll() */
/* It is intended to be connected to a serial/terminal port */
/* Type characters. If a terminator if encountered or the chunk */
/* size is read, the onDataReceived() callback will invoked. */

/* If "stop" is encountered then polling  will be terminated */

import Timer from "timer";
import Serial from "serial";

let serial = new Serial();

export default function() {
	serial.onDataReceived = function(str, len) {
		trace("got: [" + str + "]\n");
		if (-1 != str.indexOf("stop"))
			serial.poll();
	}
	serial.poll({ terminators: "\r\n", trim: 1, chunkSize: 16 });

	Timer.repeat(() => {
		serial.writeLine("tick.");
	}, 10000);
}
