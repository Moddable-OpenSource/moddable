import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import TCP from "embedded:io/socket/tcp";
import UDP from "embedded:io/socket/udp";
import Resolver from "embedded:network/dns/resolver/udp";

//import HTTPClient from "embedded:network/http/client";

const dns = {
	io: Resolver,
	servers: [
		"1.1.1.1", //...
	],
	socket: {
		io: UDP,
	},
};
const device = {
	...globalThis.device,
	network: {
		...globalThis.device?.network,
		http: {
//			io: HTTPClient,
			dns,
			socket: {
				io: TCP,
			},		
		},
	},
}

if (globalThis.device?.network?.http?.io)
	device.network.http.io = globalThis.device.network.http.io; 

globalThis.device = Object.freeze(device, true);

