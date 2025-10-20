import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import UDP from "embedded:io/socket/udp";
import DNSSD from "embedded:network/dnssd" 

globalThis.device = Object.freeze({
	...globalThis.device,
	network: {
		...globalThis.device?.network,
		dnssd: {
			io: DNSSD,
			socket: {
				io: UDP
			}		
		}
	},
}, true);
