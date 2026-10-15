import builtin from "embedded:provider/builtin";
import "config/wsclient";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.wss = {
	io: device.network.ws.io,
	dns: device.network.ws.dns,
	socket: {
		io: "TLS",
		TCP: device.network.ws.socket,
		tls: {
			applicationLayerProtocol: "http/1.1"	// handshake is HTTP/1.1 upgrade
		}
	}
};
