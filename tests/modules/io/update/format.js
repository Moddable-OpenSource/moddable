/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";

const running = flash.open({path: "running"});
let ota = update.open({partition: flash.open({path: "nextota"})});
assert.sameValue(ota.format, "buffer");
assert.throws(RangeError, () => ota.format = "buffer-x");
assert.sameValue(ota.format, "buffer");
ota.format = "buffer";
ota.close();

assert.throws(RangeError, () => flash.open({path: "nextota", format: "buffer-x"}));

ota = update.open({partition: flash.open({path: "nextota", format: "buffer"})});
assert.sameValue(ota.format, "buffer");
ota.close();
