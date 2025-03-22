/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const {promise: p, resolve, reject} = Promise.withResolvers();

const host = "www.example.com";
let wrote = false;
let t = new TCP({
	address: await $NETWORK.resolve("www.example.com"),
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
		resolve(bytes);
	},
	onError() {
		reject("connection failure")
	}
});
assert.sameValue(t.read(Uint8Array.of(1)), undefined, "read before connected");

assert(await p >= 8, "expected at least 8 bytes");
const bytes = new Uint8Array(8);
assert.sameValue(t.read(bytes), bytes.length);
assert.sameValue(String.fromArrayBuffer(bytes.buffer), "HTTP/1.1");
t.format = "number";
assert.sameValue(t.read(), 32);

$DONE();
