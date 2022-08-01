/*---
description: 
flags: [module]
---*/

import Flash from "flash";

let f = new Flash($TESTMC.config.flashParition);
let blockSize = f.blockSize;
assert.sameValue("number", typeof blockSize, "blockSize is number");
assert(blockSize > 0, "blockSize is positive");
assert.sameValue(0, blockSize & (blockSize - 1), "blockSize is power of 2");

assert.throws(TypeError, () => f.blockSize = blockSize, "blockSize is read-only");
