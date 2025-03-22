import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import TCP from "embedded:io/socket/tcp";
import UDP from "embedded:io/socket/udp";
import Resolver from "embedded:network/dns/resolver/udp";
import TLSSocket from "embedded:io/socket/tcp/tls";

import WebSocketClient from "embedded:network/websocket/client";

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
		...globalThis.device.network,
		wss: {
			io: WebSocketClient,
			dns,
			socket: {
				io: TLSSocket,
				TCP: device.network.ws.socket
			}
		},
	},
}, true);
