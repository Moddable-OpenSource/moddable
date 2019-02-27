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

import SecureSocket from "securesocket";

const host = "www.example.com";
const port = 443;

(new SecureSocket({host, port,
		secure: {cache: false, protocolVersion: 0x303}})).callback = function(message, value) {
	trace(`SOCKET MESSAGE ${message} WITH VALUE ${(undefined === value) ? "undefined" : value} \n`);

	switch (message) {
		case 1:
			this.write("GET / HTTP/1.1\r\n");
			this.write("Host: ", host, "\r\n");
			this.write("Connection: close\r\n");
			this.write("\r\n");
			break;

		case 2:
			trace(this.read(String));
			break;

		case -1:
			this.close();
			break;
	}
}
