import device from "embedded:provider/builtin";
import DNSSD from "embedded:network/dnssd";

device.network ??= {};
device.network.dnssd = {
	io: DNSSD,
	socket: {
		io: "UDP"
	}
};
