import builtin from "embedded:provider/builtin";
import "config/dns";


import NTP from "embedded:network/ntp/client";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.ntp ??= {}
device.network.ntp.client = {
	io: NTP,
	dns: device.network.dns.resolver,
	socket: {
		io: "UDP"
	},
	servers: ["pool.ntp.org"]
};
