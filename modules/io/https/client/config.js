import builtin from "embedded:provider/builtin";
import "config/httpclient";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.https ??= {};
device.network.https.client = {
	io: device.network.http.client.io,
	dns: device.network.http.client.dns,
	port: 443,
	socket: {
		io: "TLS",
		TCP: device.network.http.client.socket,
		tls: {
			applicationLayerProtocol: "http/1.1"
		}
	}
};
