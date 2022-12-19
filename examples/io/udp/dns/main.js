/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import Resolver from "embedded:network/dns/resolver/udp";
import UDP from "embedded:io/socket/udp";
import Timer from "timer";
import Net from "net";

const resolver = new Resolver({
	socket: {
		io: UDP
	},
	servers: Net.get("DNS") ?? ["8.8.8.8", "1.1.1.1"]
});

function resolve(host, callback) {
	resolver.resolve({
		host,
		onResolved: (host, address) => callback?.(address), 
		onError: (host) => callback?.(),
	});
}

Timer.set(() => {
	resolve("moddable.com", address => trace(`Moddable ${address}\n`));
	resolve("yahoo.com", address => trace(`Yahoo ${address}\n`));
	resolve("Desk-Lamp.local", address => trace(`Desk-Lamp ${address}\n`));
	resolve("ffffasdfasdfgsgasdfasdf.net", address => trace(`global garbage ${address}\n`));
	resolve("ffffasdfasdfgsgasdfasdf.local", address => trace(`local garbage ${address}\n`));
}, 1, 10000);
