/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const address = await $NETWORK.resolve("www.example.com");

new TCP({
	address,
	port: 80,
	onWritable(bytes) {
		$DO(() => {
			assert.sameValue(this.remoteAddress, address, "bad remoteAddress");
			assert.throws(TypeError, () => {this.remoteAddress = "abcd";}); 
			assert.sameValue(this.remoteAddress, address, "remoteAddress changed by attempted write");
		})();
	},
	onError() {
		$DONE("error connecting");
	}
});
