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

import Serial from "builtin/serial";

const msg = "hello world\r\n";

if (1) {		// using buffers
	let serial = new Serial({
		baud: 115200 * 8,
		onReadable: function(count) {
			this.write(this.read());
		},
	});
	serial.format = "buffer";

	serial.write(ArrayBuffer.fromString(msg));
}
else {		// using bytes
	let serial = new Serial({
		baud: 115200 * 8,
		onReadable: function(count) {
			while (count--)
				this.write(this.read());
		},
	});

	for (let i = 0; i < msg.length; i++)
		serial.write(msg.charCodeAt(i));
}
