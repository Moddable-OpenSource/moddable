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

import {Socket, Listener} from "socket";

let count = 0;
(new Listener({port: 8080})).callback = function() {
	let socket = new Socket({listener: this});
	socket.callback = function (message) {
		if (3 === message)
			this.close();
	}

	const message = `Hello, server ${++count}.`;
	socket.write("HTTP/1.1 200 OK\r\n",
				"Connection: close\r\n",
				`Content-Length: ${message.length}\r\n`,
				"Content-Type: text/plain\r\n",
				"\r\n",
				message);

	trace(`listener responded with message ${count}\n`);
}

trace("listener ready\n");
