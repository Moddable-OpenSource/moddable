/*---
description: >
  Array.prototype.flat accepts depths above 2**32 - 1. fx_Array_prototype_flat
  (xsArray.c) cast the depth and the length to txIndex, which is undefined
  behavior above 2**32 - 1; a wrapping conversion turns a depth of 2**32 into 0.
includes: [compareArray.js]
flags: [onlyStrict]
---*/

assert.compareArray([[[1]]].flat(2 ** 32), [1], "depth 2**32");
assert.compareArray([[[1]]].flat(2 ** 32 + 1), [1], "depth 2**32 + 1");
assert.compareArray([[[1]]].flat(2 ** 53), [1], "depth 2**53");
assert.compareArray([[[1]]].flat(Infinity), [1], "depth Infinity");
assert.sameValue([[[1]]].flat(-(2 ** 32))[0][0][0], 1, "negative depth does not flatten");
