/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const target = {};
const port = 8080;
const address = "127.0.0.1";
const l = new Listener({
	port,
	target,
	onReadable(count) {
		$DO(() => {
			assert.sameValue(this.target, target, "bad target in callback");
		})();
	}
});

new TCP({
	address,
	port
});

assert.sameValue(l.target, target, "bad target after instantiation");
