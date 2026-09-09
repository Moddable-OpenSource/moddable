import device from "embedded:provider/builtin";
import UDP from "embedded:io/socket/udp";
import DNSSD from "embedded:network/dnssd";

device.network ??= {};
device.network.dnssd = {
	io: DNSSD,
	socket: {
		io: UDP
	}
};
