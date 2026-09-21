import device from "embedded:provider/builtin";
import HTTPServer from "embedded:network/http/server";

device.network ??= {};
device.network.http ??= {};
device.network.http.server = {
	io: HTTPServer,
	socket: {
		io: "Listener"
	}
};
