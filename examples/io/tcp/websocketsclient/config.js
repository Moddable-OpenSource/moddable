import device from "embedded:provider/builtin";
import "config/wsclient";
import TLSSocket from "embedded:io/socket/tcp/tls";

device.network ??= {};
device.network.wss = {
	io: device.network.ws.io,
	dns: device.network.ws.dns,
	socket: {
		io: TLSSocket,
		TCP: device.network.ws.socket,
		tls: {
			applicationLayerProtocol: "http/1.1"	// handshake is HTTP/1.1 upgrade
		}
	}
};
