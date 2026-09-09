import device from "embedded:provider/builtin";
import "config/dns";
import TCP from "embedded:io/socket/tcp";
import TLSSocket from "embedded:io/socket/tcp/tls";
import MQTTClient from "embedded:network/mqtt/client";

device.network ??= {};
device.network.mqtts = {
	io: MQTTClient,
	dns: device.network.dns.resolver,
	socket: {
		io: TLSSocket,
		TCP: {
			io: TCP
		},
		tls: {
			applicationLayerProtocol: "mqtt"
		}
	}
};
