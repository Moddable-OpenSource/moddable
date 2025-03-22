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
		$DO(() => {
			assert.sameValue(1, count, "expect one readable socket")
			this.close();
		})();
	}
});

assert.sameValue(l.read(), undefined, "nothing to read initially");

new TCP({
	address,
	port: l.port
});
