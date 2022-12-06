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

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

let requests = 0;
new Listener({
	port: 80,
	onReadable(count) {
		while (count--) {
			const socket = new TCP({
				from: this.read(),
				onReadable() {
					const echo = this.read();
					const msg = ArrayBuffer.fromString(`\r\n\r\nResponse ${++requests}. ${new Date} ${this.remoteAddress}:${this.remotePort}`);

					this.write(ArrayBuffer.fromString("HTTP/1.1 200 OK\r\n"));
					this.write(ArrayBuffer.fromString("connection: close\r\n"));
					this.write(ArrayBuffer.fromString("content-type: text/plain\r\n"));
					this.write(ArrayBuffer.fromString(`content-length: ${echo.byteLength + msg.byteLength}\r\n`));
					this.write(ArrayBuffer.fromString("\r\n"));
					this.write(echo);
					this.write(msg);

					this.close();
				}
			});
		}
	}
});
