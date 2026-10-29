/*---
description:
flags: [module]
---*/

import Digest from "./digest_FIXTURE.js";

const d = new Digest({algorithm: "SHA-256"});

// non-buffer arguments: all caught by xsmcGetBufferReadable → TypeError
assert.throws(TypeError, () => d.write("abc"),     "string");
assert.throws(TypeError, () => d.write(42),        "number");
assert.throws(TypeError, () => d.write(null),      "null");
assert.throws(TypeError, () => d.write(undefined), "undefined");
assert.throws(TypeError, () => d.write({}),        "plain object");
assert.throws(TypeError, () => d.write([1,2,3]),   "plain array");

d.close();
