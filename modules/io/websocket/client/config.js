import builtin from "embedded:provider/builtin";
import "config/dns";
import WebSocketClient from "embedded:network/websocket/client";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.ws = {
	io: WebSocketClient,
	dns: device.network.dns.resolver,
	socket: {
		io: "TCP"
	}
};
