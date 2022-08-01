/*---
description: 
flags: [module]
---*/

import Flash from "flash";

const f = new Flash($TESTMC.config.flashParition);
const {byteLength, blockSize} = f;

assert.throws(SyntaxError, () => f.read(), "read requires 2 arguments");
assert.throws(SyntaxError, () => f.read(1), "read requires 2 arguments");

assert.throws(RangeError, () => f.read(0, -1), "read length must be non-negative");
assert.sameValue(f.read(0, 0).byteLength, 0, "0 length read");

assert.throws(Error, () => f.read(-1, 1), "read offset must be non-negative");
assert.throws(Error, () => f.read(byteLength, 1), "read offset must less than byteLength");
assert.throws(Error, () => f.read(byteLength + 1, 1), "read offset must less than byteLength");

assert.throws(Error, () => f.read(byteLength - 1, 2), "read end must be before byteLength");
assert.sameValue(f.read(byteLength - 64, 64).byteLength, 64, "read to end of partition");

assert.sameValue(f.read("0", "4").byteLength, 4, "read coerces arguments to numbers");
assert(f.read(0, 10) instanceof ArrayBuffer, "read returns ArrayBuffer");

assert.throws(SyntaxError, () => f.read.call(new $TESTMC.HostObject, 0, 64), "read with non-flash this");
