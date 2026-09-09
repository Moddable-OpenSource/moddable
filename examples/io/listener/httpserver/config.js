import device from "embedded:provider/builtin";
import HTTPServer from "embedded:network/http/server";
import Listener from "embedded:io/socket/listener";

device.network ??= {};
device.network.http ??= {};
device.network.http.server = {
	io: HTTPServer,
	socket: {
		io: Listener
	}
};
