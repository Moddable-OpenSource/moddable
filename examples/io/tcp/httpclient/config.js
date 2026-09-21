import device from "embedded:provider/builtin";
import "config/dns";
import HTTPClient from "embedded:network/http/client";

device.network ??= {};
device.network.http ??= {};
device.network.http.client = {
	io: HTTPClient,
	dns: device.network.dns.resolver,
	socket: {
		io: "TCP"
	}
};
