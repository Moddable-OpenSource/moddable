/*---
description:
flags: [module]
---*/

import Random from "./random_FIXTURE.js";

using r = new Random({});

// the count is coerced as ArrayBuffer coerces it; ArrayBuffer rejects what cannot be allocated
assert.throws(RangeError, () => r.read(-1), "negative count");
assert.throws(RangeError, () => r.read(Infinity), "Infinity count");
assert.sameValue(r.read(1.5).byteLength, 1, "fractional count truncates");
assert.sameValue(r.read(NaN).byteLength, 0, "NaN is 0");
assert.sameValue(r.read("16").byteLength, 16, "numeric string");
assert.sameValue(r.read().byteLength, 0, "no argument is 0");

// objects must be byte buffers
assert.throws(TypeError, () => r.read({}), "plain object");
assert.throws(TypeError, () => r.read([1, 2, 3]), "array");
assert.throws(TypeError, () => r.read(new Float32Array(4)), "Float32Array is not a byte buffer");
