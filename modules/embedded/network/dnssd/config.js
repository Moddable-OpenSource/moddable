import builtin from "embedded:provider/builtin";
import DNSSD from "embedded:network/dnssd";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.dnssd = {
	io: DNSSD,
	socket: {
		io: "UDP"
	}
};
