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

import {Server} from "http"

let server = new Server({});
server.callback = function(message, value)
{
	if (Server.status === message)
		this.path = value;

	if (Server.prepareResponse === message)
		return {headers: ["Content-type", "text/plain"], body: true};

	if (Server.responseFragment === message) {
		let i = Math.round(Math.random() * 20);
		if (0 == i)
			return;
		if (1 == i)
			i = this.path;
		return i + "\n";
	}
}
