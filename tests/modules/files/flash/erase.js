/*---
description: 
flags: [module]
---*/

import Flash from "flash";

const f = new Flash($TESTMC.config.flashParition);
const {byteLength, blockSize} = f;
const blocks = byteLength / blockSize;

assert.throws(SyntaxError, () => f.erase(), "erase requires 1 arguments");

assert.throws(Error, () => f.erase(-1), "erase block must be non-negative");
assert.throws(Error, () => f.erase(blocks), "erase block less than block count");
assert.throws(Error, () => f.erase(blocks + 1), "erase block less than block count");

f.erase(0);
f.erase(String(blocks - 1));

assert.throws(SyntaxError, () => f.erase.call(new $TESTMC.HostObject, 1), "erase with non-flash this");
