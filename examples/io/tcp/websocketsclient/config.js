import device from "embedded:provider/builtin";
import "config/wsclient";

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
