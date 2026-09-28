import builtin from "embedded:provider/builtin";
import "config/dns";
import MQTTClient from "embedded:network/mqtt/client";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.mqtt = {
	io: MQTTClient,
	dns: device.network.dns.resolver,
	socket: {
		io: "TCP"
	}
};
