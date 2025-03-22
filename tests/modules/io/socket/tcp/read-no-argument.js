/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const host = "www.example.com";
let wrote = false;
new TCP({
	address: await $NETWORK.resolve(host),
	port: 80,
	onWritable(bytes) {
		if (wrote)
			return;
		wrote = true;

		const headers = [
			`GET / HTTP/1.1`,
			`Host: ${host}`,
			"Connection: close",
			"",
			"",
		].join("\r\n")
		this.write(ArrayBuffer.fromString(headers));
	},
	onReadable(bytes) {
		$DO(() => {
			let result = this.read();

			assert(result instanceof ArrayBuffer, "expect Arrray Buffer");
			assert(result.byteLength >= bytes, "expect at least bytes to be read");

			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});
