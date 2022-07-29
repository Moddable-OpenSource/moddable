/*---
description: 
flags: [module]
---*/

import Flash from "flash";

let f = new Flash($TESTMC.config.flashParition);
let byteLength = f.byteLength;
assert.sameValue("number", typeof byteLength, "byteLength is number");
assert(byteLength > 0, "byteLength is positive");
assert.sameValue(0, byteLength % f.blockSize, "byteLength is multiple of blockSize");

assert.throws(TypeError, () => f.byteLength = byteLength, "byteLength is read-only");
