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

import WiFi from "wifi";
import {Server} from "http"
import Net from "net"

WiFi.accessPoint({
	ssid: "South Village",
	password: "12345678",
	channel: 8,
	hidden: false
});

(new Server({port: 80})).callback = function(message, value) {
	if (Server.status === message)
		this.path = value;

	if (Server.prepareResponse === message)
		return {headers: ["Content-type", "text/plain"], body: `hello, client at path ${this.path}.`};
}

trace(`http server ready at ${Net.get("IP")}\n`);

