import device from "embedded:provider/builtin";
import "config/httpclient";
import TLSSocket from "embedded:io/socket/tcp/tls";

device.network ??= {};
device.network.https ??= {};
device.network.https.client = {
	io: device.network.http.client.io,
	dns: device.network.http.client.dns,
	port: 443,
	socket: {
		io: TLSSocket,
		TCP: device.network.http.client.socket,
		tls: {
			applicationLayerProtocol: "http/1.1"
		}
	}
};
