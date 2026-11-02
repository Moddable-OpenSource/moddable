import builtin from "embedded:provider/builtin";
import Resolver from "embedded:network/dns/resolver/udp";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.dns ??= {};
device.network.dns.resolver = {
	io: Resolver,
	servers: ["1.1.1.1", "8.8.8.8"],
	socket: {
		io: "UDP"
	}
};
