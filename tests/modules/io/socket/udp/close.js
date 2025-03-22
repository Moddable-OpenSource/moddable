/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

const address = "127.0.0.1";
const port = 80;

$TESTMC.timeout(5_000);

const u = new UDP({
});

u.close();
u.close();

assert.throws(SyntaxError, () => u.read());
assert.throws(SyntaxError, () => u.write(address, port, Uint8Array.of(1)));

$DONE();
