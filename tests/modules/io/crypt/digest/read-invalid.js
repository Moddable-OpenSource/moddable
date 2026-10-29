/*---
description:
flags: [module]
---*/

import Digest from "./digest_FIXTURE.js";

const d = new Digest({algorithm: "SHA-256"});

// buffer too small for outputSize (SHA-256 needs 32)
assert.throws(RangeError, () => d.read(new ArrayBuffer(0)),  "0-byte buffer");
assert.throws(RangeError, () => d.read(new ArrayBuffer(31)), "31-byte buffer");
assert.throws(RangeError, () => d.read(new Uint8Array(10)),  "10-byte Uint8Array");

// non-buffer arguments: xsmcGetBufferWritable rejects with TypeError
assert.throws(TypeError, () => d.read("abc"),     "string");
assert.throws(TypeError, () => d.read(42),        "number");
assert.throws(TypeError, () => d.read(null),      "null");
assert.throws(TypeError, () => d.read(undefined), "undefined");
assert.throws(TypeError, () => d.read({}),        "plain object");

d.close();
