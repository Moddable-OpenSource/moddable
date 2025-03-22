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
			assert.sameValue(typeof bytes, "number", "expect number");
			assert(bytes > 0, "expect at least one writable byte");
			assert.sameValue(this.format, "buffer", "expect default format to be buffer");

			assert.throws(TypeError, () => this.read(Symbol.match));

			let result = this.read(4);
			assert(result instanceof ArrayBuffer, "expect ArrayBuffer");
			assert.sameValue(result.byteLength, 4);
			assert.sameValue(String.fromArrayBuffer(result), "HTTP");

			result = new Uint8Array(4);
			assert.sameValue(this.read(result), 4);
			assert.sameValue(String.fromArrayBuffer(result.buffer), "/1.1");
			
			this.format = "number";
			assert.sameValue(this.read(), ' '.charCodeAt());

			this.format = "buffer";
			assert.throws(TypeError, () => this.read(Uint16Array.of(2)));
			assert.throws(TypeError, () => this.read(Math.sin));

			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});
