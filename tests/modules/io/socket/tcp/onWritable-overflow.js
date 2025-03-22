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
		// "bytes" is the minumum numbers of bytes that can be written.
		// the actual number may be larger because the network buffers may 
		// drain during the execution of this function, freeing up more space
		$DO(() => {
			assert.throws(Error, () => this.write(new ArrayBuffer(bytes + 1)), "overflow 1");

			this.format = "number";
			for (let i = 0; i < bytes - 1; i++)
				this.write(32);

			this.format = "buffer";
			this.write(Uint8Array.of(32));

			assert.throws(Error, () => this.write(new Uint8Array(bytes)), "overflow 2");

			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});
