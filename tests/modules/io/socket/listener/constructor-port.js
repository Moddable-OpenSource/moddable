/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

assert.throws(Error, () => {new Listener({
		port: undefined
	});
}, "port of undefined should be rejected");

assert.throws(RangeError, () => {new Listener({
		port: -1
	});
}, "port of -1");

assert.throws(RangeError, () => {new Listener({
		port: 65536
	});
}, "port of 65536");

assert.throws(TypeError, () => {new Listener({
		port: Symbol.match
	});
}, "port of Symbol.match");

let l = new Listener({
	port: "0"
});
assert("number" === typeof l.port, "port should be number");
assert((1 <= l.port) && (l.port <= 65535), "port out of range: " + l.port);
l.close();

l = new Listener({
});
assert("number" === typeof l.port, "default port should be number");
assert((1 <= l.port) && (l.port <= 65535), "default port out of range: " + l.port);
l.close();


$DONE();
