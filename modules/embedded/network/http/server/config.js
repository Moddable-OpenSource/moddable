import builtin from "embedded:provider/builtin";
import HTTPServer from "embedded:network/http/server";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.network ??= {};
device.network.http ??= {};
device.network.http.server = {
	io: HTTPServer,
	socket: {
		io: "Listener"
	}
};
