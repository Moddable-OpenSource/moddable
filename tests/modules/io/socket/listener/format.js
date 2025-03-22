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

const l = new Listener({
	port,
	onReadable(count) {
		$DO(() => {
			assert.sameValue(this.format, "socket/tcp");
		})();
	}
});

assert.sameValue(l.format, "socket/tcp");
assert.throws(RangeError, () => l.format = "123");
assert.sameValue(l.format, "socket/tcp");

new TCP({
	address,
	port
});
