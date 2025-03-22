/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

let u = new UDP({});
assert.sameValue(u.format, "buffer");
u.close();

u = new UDP({format: "buffer"});
assert.sameValue(u.format, "buffer");
u.close();

assert.throws(RangeError, () => new UDP({format: "number"}));
assert.throws(RangeError, () => new UDP({format: "INVALID FORMAT"}));

$DONE()
