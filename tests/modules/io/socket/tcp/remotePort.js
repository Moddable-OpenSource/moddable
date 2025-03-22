/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const port = 80;
new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port,
	onWritable(bytes) {
		$DO(() => {
			assert.sameValue(this.remotePort, port, "bad remotePort");
			assert.throws(TypeError, () => {this.remotePort = -123;}); 
			assert.sameValue(this.remotePort, port, "remotePort changed by attempted write");
		})();
	},
	onError() {
		$DONE("error connecting");
	}
});

