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

import {Server} from "websocket"
import Net from "net";

let server = new Server({port:80});
server.callback = function (message, value)
{
	switch (message) {
		case 1:
			trace("main.js: socket connect.\n");
			break;

		case 2:
			trace("main.js: websocket handshake success\n");
			break;

		case 3:
			trace(`main.js: websocket message received: ${value}\n`);
			this.write(value);		// echo
			break;

		case 4:
			trace("main.js: websocket close\n");
			break;
	}
}

trace(`web socket server ready at ${Net.get("IP")}\n`);
