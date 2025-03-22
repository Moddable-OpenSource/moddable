/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const target = {};

const t = new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	target,
	onWritable(bytes) {
		$DO(() => {
			assert.sameValue(this.target, target, "bad target in callback");
		})();
	},
	onError() {
		$DONE("error connecting");
	}
});

assert.sameValue(t.target, target, "bad target after instantiation");
