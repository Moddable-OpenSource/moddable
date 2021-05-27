/*
 * Copyright (c) 2019-2020  Moddable Tech, Inc.
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

const msg = "hello world\r\n";

if (1) {		// using buffers
	let serial = new device.io.Serial({
		...device.Serial.default,
		baud: 115200,
		port: 2,
		format: "buffer",
		onReadable: function(count) {
			this.write(this.read());
		},
	});

	serial.write(ArrayBuffer.fromString(msg));
}
else {		// using bytes
	let serial = new device.io.Serial({
		...device.Serial.default,
		baud: 115200 * 8,
		onReadable: function(count) {
			while (count--)
				this.write(this.read());
		},
	});

	for (let i = 0; i < msg.length; i++)
		serial.write(msg.charCodeAt(i));
}
