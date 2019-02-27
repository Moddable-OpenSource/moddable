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

import {Socket} from "socket";

let messageNames = ["error", "disconnect", "unused", "connect", "dataReceived", "dataSent"];

let bytes = 0;

let host = "www.example.com";
let socket = new Socket({host: host, port: 80});
socket.callback = function(message, value)
{
	trace(`Socket message ${messageNames[message + 2]}`);
	if (undefined != value)
		trace(` VALUE = ${value}`);
	trace("\n");

	if (1 == message) {
		this.write("GET / HTTP/1.1\r\n");
		this.write("Host: ", host, "\r\n");
		this.write("Connection: close\r\n");
		this.write("\r\n");
	}
	else if (2 == message) {
		bytes += value;

		trace(this.read(String));
		trace("\n");
	}
	else if (-1 == message)
		trace(`socket disconnected after receiving ${bytes} bytes in http response.\n`);
}

trace(`socket created.\n`);
