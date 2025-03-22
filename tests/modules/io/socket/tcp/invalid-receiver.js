/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const {promise: writable, resolve: resolveWrite, reject: rejectWrite} = Promise.withResolvers();
const {promise: readable, resolve: resolveRead, reject: rejectRead} = Promise.withResolvers();

const host = "www.example.com";
let wrote = false;
let t = new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	onWritable(bytes) {
		if (wrote)
			return;
		wrote = true;
		resolveWrite(bytes);

	},
	onReadable(bytes) {
		resolveRead(bytes);
	},
	onError() {
		rejectWrite("connection failure")
		rejectRead("connection failure")
	}
});


const headers = [
	`GET / HTTP/1.1`,
	`Host: ${host}`,
	"Connection: close",
	"",
	"",
].join("\r\n")

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName} with array`);
}

assert(await writable >= 8, "expected at least 8 bytes writable");

callWithInvalidReceivers(t, "write", ArrayBuffer.fromString(headers));
t.write(ArrayBuffer.fromString(headers));

assert(await readable >= 8, "expected at least 8 bytes readable");

const bytes = new Uint8Array(8);
callWithInvalidReceivers(t, "read", bytes);
assert.sameValue(t.read(bytes), bytes.length);
assert.sameValue(String.fromArrayBuffer(bytes.buffer), "HTTP/1.1");
t.format = "number";
callWithInvalidReceivers(t, "read");
assert.sameValue(t.read(), 32);

callWithInvalidReceivers(t, "close");
t.close();

$DONE();
