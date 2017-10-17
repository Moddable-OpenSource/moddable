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

import {Client} from "websocket"

let ws = new Client({host: "echo.websocket.org"});

ws.callback = function(message, value)
{
	switch (message) {
		case 1:
			trace("socket connect\n");
			break;

		case 2:
			trace("websocket handshake success\n");
			this.write(JSON.stringify({count: 1, toggle: true}));
			break;

		case 3:
			trace(`websocket message received: ${value}\n`);
			value = JSON.parse(value);
			value.count += 1;
			value.toggle = !value.toggle;
			value.random = Math.round(Math.random() * 1000);
			this.write(JSON.stringify(value));
			break;

		case 4:
			trace("websocket close\n");
			break;
	}
}
