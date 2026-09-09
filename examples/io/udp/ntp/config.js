import device from "embedded:provider/builtin";
import "config/dns";

import UDP from "embedded:io/socket/udp";		// the NTP client's own transport

import NTP from "embedded:network/ntp/client";

device.network ??= {};
device.network.ntp ??= {}
device.network.ntp.client = {
	io: NTP,
	dns: device.network.dns.resolver,
	socket: {
		io: UDP
	},
	servers: ["pool.ntp.org"]
};
