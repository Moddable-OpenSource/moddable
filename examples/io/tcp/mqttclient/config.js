import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import TCP from "embedded:io/socket/tcp";
import UDP from "embedded:io/socket/udp";
import Resolver from "embedded:network/dns/resolver/udp";

import MQTTClient from "embedded:network/mqtt/client";

const dns = {
	io: Resolver,
	servers: [
		"1.1.1.1", //...
	],
	socket: {
		io: UDP,
	},
};
globalThis.device = Object.freeze({
	...globalThis.device,
	network: {
		...globalThis.device?.network,
		mqtt: {
			io: MQTTClient,
			dns,
			socket: {
				io: TCP,
			},		
		},
	},
}, true);

