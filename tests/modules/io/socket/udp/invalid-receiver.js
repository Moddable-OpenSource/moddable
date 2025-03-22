/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const {promise: readable, resolve: resolveRead, reject: rejectRead} = Promise.withResolvers();

const address = "127.0.0.1";
const port = 51515;
let u = new UDP({
	port,
	onReadable(bytes) {
		resolveRead(bytes);
	}
});

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName} with array`);
}

callWithInvalidReceivers(u, "write", address, port, new ArrayBuffer(1));
u.write(address, port, new ArrayBuffer(1));

assert(await readable >= 1, "expected at least 1 packet readable");

callWithInvalidReceivers(u, "read");
assert.sameValue(u.read().byteLength, 1);

callWithInvalidReceivers(u, "close");
u.close();

$DONE();
