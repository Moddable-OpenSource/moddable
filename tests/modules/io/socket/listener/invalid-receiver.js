/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const {promise: readable, resolve: resolveRead, reject: rejectRead} = Promise.withResolvers();

const address = "127.0.0.1";
const port = 8080;

const l = new Listener({
	port,
	onReadable(count) {
		resolveRead(count);
	}
});

callWithInvalidReceivers(l, "read");

new TCP({
	address,
	port
});

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName} with array`);
}

const count = await readable;;
assert.sameValue(count, 1, "expect 1 socket to read");

callWithInvalidReceivers(l, "read");
assert(l.read() instanceof TCP, "read should return tcp socket");

callWithInvalidReceivers(l, "close");
l.close();

$DONE();
