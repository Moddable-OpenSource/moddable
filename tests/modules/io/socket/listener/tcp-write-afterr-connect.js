/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const address = "127.0.0.1";

const l = new Listener({
	onReadable(count) {
		new TCP({
			from: this.read(),
			onWritable(count) {
				$DO(() => {
					this.write(Uint8Array.of(1, 2, 3, 4));
					this.close();
					l.close();
				})();
			}
		});
	}
});

new TCP({
	address,
	port: l.port
});
