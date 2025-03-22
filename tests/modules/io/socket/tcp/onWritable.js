/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	onWritable(bytes) {
		$DO(() => {
			assert.sameValue(typeof bytes, "number", "expect number");
			assert(bytes > 0, "expect at least one writable byte");
			assert.sameValue(this.format, "buffer", "expect default format to be buffer");
			assert.throws(TypeError, () => this.write(32));
			this.write(new ArrayBuffer(12));
			assert.throws(TypeError, () => this.write(Symbol.match));
			this.format = "number";
			this.write(32);
			assert.throws(TypeError, () => this.write(Symbol.match));
			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});
