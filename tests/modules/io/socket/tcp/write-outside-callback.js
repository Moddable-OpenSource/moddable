/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const {promise: p, resolve, reject} = Promise.withResolvers();

let t = new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	onWritable(bytes) {
		resolve(bytes);
	},
	onError() {
		reject("connection failure")
	}
});
assert.throws(Error, () => t.write(Uint8Array.of(1)), "write before connected");

const bytes = await p;
assert.sameValue(typeof bytes, "number");
assert(bytes > 0, "need at least 1 byte writable");
t.write(new ArrayBuffer(bytes));

$DONE();
