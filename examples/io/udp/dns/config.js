import device from "embedded:provider/builtin";
import UDP from "embedded:io/socket/udp";
import Resolver from "embedded:network/dns/resolver/udp";

device.network ??= {};
device.network.dns ??= {};
device.network.dns.resolver = {
	io: Resolver,
	servers: ["1.1.1.1", "8.8.8.8"],
	socket: {
		io: UDP
	}
};
