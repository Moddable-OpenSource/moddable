
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
