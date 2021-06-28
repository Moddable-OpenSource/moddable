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

const message = ArrayBuffer.fromString("Since publication of the first edition in 1997, ECMAScript has grown to be one of the world's most widely used general-purpose programming languages. It is best known as the language embedded in web browsers but has also been widely adopted for server and embedded applications.\r\n");
let offset = 0;

new device.io.Serial({
	...device.Serial.default,
	baud: 921600 / 2,
	format: "buffer",
	onWritable: function(count) {
		do {
			const use = Math.min(count, message.byteLength - offset);
			this.write(message.slice(offset, offset + use));
			count -= use;
			offset += use;
			if (offset >= message.byteLength)
				offset = 0;
		} while (count);
	}
});
