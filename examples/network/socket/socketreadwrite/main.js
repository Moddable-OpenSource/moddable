/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import {Socket} from "socket";

let bytes = 0;

const host = "www.example.com";
const socket = new Socket({host, port: 80});
socket.callback = function(message, value)
{
	let name;

	for (name in Socket) {
		if (Socket[name] === message)
			break;
	}

	trace(`Socket message: ${name} `);
	if (undefined !== value)
		trace(value);
	trace("\n");

	if (Socket.connected === message) {
		this.write("GET / HTTP/1.1\r\n");
		this.write("Host: ", host, "\r\n");
		this.write("Connection: close\r\n");
		this.write("\r\n");
	}
	else if (Socket.readable === message) {
		bytes += value;

		trace(this.read(String), "\n");
	}
	else if (message < 0)
		trace(`socket disconnected after receiving ${bytes} bytes in http response.\n`);
}

trace(`socket created.\n`);
