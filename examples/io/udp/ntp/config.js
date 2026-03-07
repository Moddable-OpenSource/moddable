import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import UDP from "embedded:io/socket/udp";
import Resolver from "embedded:network/dns/resolver/udp";

import NTP from "embedded:network/ntp/client";

const dns = {
	io: Resolver,
	servers: [
		"1.1.1.1", //...
	],
	socket: {
		io: UDP
	}
};
globalThis.device = Object.freeze({
	...globalThis.device,
	network: {
		...globalThis.device?.network,
		ntp: {
			client: {
				io: NTP,
				dns,
				socket: {
					io: UDP
				},
				servers: ["pool.ntp.org"]
			}
		}
	},
}, true);
