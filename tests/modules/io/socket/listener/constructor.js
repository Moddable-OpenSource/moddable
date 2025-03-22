/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const port = 8080;
const address = "127.0.0.1";

let pending = 3;

new Listener({
	port,
	onReadable(count) {
		while (count--) {
			const tcp = this.read();
			if (!(tcp instanceof TCP))
				$DONE("read should return TCP instance");
			if (tcp.remoteAddress !== address)
				$DONE("expected remoteAddress " + address);
			if (tcp.format !== "buffer")
				$DONE("expected format of buffer");
		}
	}
});

for (let i = 0; i < pending; i++) {
	new TCP({
		address,
		port,
		onWritable() {
			if (this.writable) return;
			this.writable = true;
			if (this.remotePort !== port)
				$DONE("expected reportPort " + port);
			if (this.remoteAddress !== address)
				$DONE("expected remoteAddress " + address);
			if (this.format !== "buffer")
				$DONE("expected format of buffer");
			if (0 === --pending)
				$DONE();
		},
		onError() {
			$DONE("error connecting");
		}
	});
}
