/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const address = await $NETWORK.resolve("www.example.com");
const port = 80;

let t = new TCP({address, port});
assert.sameValue(t.format, "buffer");
t.close();

t = new TCP({address, port, format: "buffer"});
assert.sameValue(t.format, "buffer");
t.close();

t = new TCP({address, port, format: "number"});
assert.sameValue(t.format, "number");
t.close();

assert.throws(RangeError, () => new TCP({address, port, format: "INVALID FORMAT"}));

$DONE()
