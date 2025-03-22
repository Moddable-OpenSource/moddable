/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const address = await $NETWORK.resolve("www.example.com");

assert.throws(Error, () => {new TCP({
		address,
	});
}, "missing port");

assert.throws(Error, () => {new TCP({
		address,
		port: undefined
	});
}, "port of undefined");

assert.throws(RangeError, () => {new TCP({
		address,
		port: -1
	});
}, "port of -1");

assert.throws(RangeError, () => {new TCP({
		address,
		port: 65536
	});
}, "port of 65536");

assert.throws(TypeError, () => {new TCP({
		address,
		port: Symbol.match
	});
}, "port of Symbol.match");

$DONE();
