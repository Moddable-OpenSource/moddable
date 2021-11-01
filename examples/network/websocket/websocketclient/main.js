/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

import {Client} from "websocket"

let ws = new Client({host: "websockets.chilkat.io", path: "/wsChilkatEcho.ashx"});

ws.callback = function(message, value) {
	switch (message) {
		case Client.connect:
			trace("socket connect\n");
			break;

		case Client.handshake:
			trace("websocket handshake success\n");
			this.write(JSON.stringify({count: 1, toggle: true}));
			break;

		case Client.receive:
			trace(`websocket message received: ${value}\n`);
			value = JSON.parse(value);
			value.count += 1;
			value.toggle = !value.toggle;
			value.random = Math.round(Math.random() * 1000);
			this.write(JSON.stringify(value));
			break;

		case Client.disconnect:
			trace("websocket close\n");
			break;
	}
}
