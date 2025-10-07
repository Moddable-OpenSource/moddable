import "system"		// system initializes globalThis.device. this ensures it runs before this module.

import UDP from "embedded:io/socket/udp";
import MDNS from "embedded:network/mdns" 

globalThis.device = Object.freeze({
	...globalThis.device,
	network: {
		...globalThis.device?.network,
		mdns: {
			io: MDNS,
			socket: {
				io: UDP
			}		
		}
	},
}, true);
