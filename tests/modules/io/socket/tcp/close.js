/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

let closed = false;
new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	onWritable(bytes) {
		if (closed)
			$DONE("onWritable invoked after close");

		$DO(() => {
			this.close();
			closed = true;
			this.close();

			assert.throws(SyntaxError, () => this.write(Uint8Array.of(1)));
			assert.throws(SyntaxError, () => this.read());
			assert.throws(SyntaxError, () => this.remoteAddress);
			assert.throws(SyntaxError, () => this.remotePort);
			assert.throws(SyntaxError, () => this.format);
		})();
	},
	onReadable(bytes) {
		if (closed)
			$DONE("onReadable invoked after close");
	},
	onError() {
		if (closed)
			$DONE("onError invoked after close");
	}
});
