import device from "embedded:provider/builtin";
import "config/dns";
import MQTTClient from "embedded:network/mqtt/client";

device.network ??= {};
device.network.mqtt = {
	io: MQTTClient,
	dns: device.network.dns.resolver,
	socket: {
		io: "TCP"
	}
};
