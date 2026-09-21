import device from "embedded:provider/builtin";
import "config/dns";
import WebSocketClient from "embedded:network/websocket/client";

device.network ??= {};
device.network.ws = {
	io: WebSocketClient,
	dns: device.network.dns.resolver,
	socket: {
		io: "TCP"
	}
};
