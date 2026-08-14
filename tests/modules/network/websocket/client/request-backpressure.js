/*---
description: WebSocket upgrade request respects socket write capacity
flags: [async, module]
---*/
import Timer from "timer";
import WebSocketClient from "embedded:network/websocket/client";

const capacity = 64;
let client;
class Resolver {
	resolve(options) {
		options.onResolved(options.host, "127.0.0.1");
	}
}
class Socket {
	#options;
	static writes = 0;

	constructor(options) {
		this.#options = options;
		Timer.set(() => options.onWritable(capacity));
	}
	write(buffer) {
		assert(buffer.byteLength <= capacity, "write exceeds reported capacity");
		Socket.writes++;
		const bytes = new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);
		const length = bytes.byteLength;
		if ((length >= 4) && (13 === bytes[length - 4]) && (10 === bytes[length - 3]) &&
			(13 === bytes[length - 2]) && (10 === bytes[length - 1])) {
			Timer.set(() => $DO(() => {
				assert(Socket.writes > 1, "request should be split across writes");
				client.close();
			})());
		}
		else
			Timer.set(() => this.#options.onWritable(capacity));
		return 0;
	}
	close() {}
	set format(_) {}
}
$TESTMC.timeout(1_000);
client = new WebSocketClient({
	host: "example.com",
	headers: [["X-Padding", "x".repeat(160)]],
	dns: {io: Resolver},
	socket: {io: Socket},
	onError() { $DONE("unexpected error"); }
});
