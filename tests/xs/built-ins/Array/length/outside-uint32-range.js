/*---
description: >
  Array lengths outside the uint32 range throw a RangeError.
  fxCheckArrayLength (xsArray.c) casts the length to txIndex before checking
  it, which is undefined behavior for negative, too large or NaN lengths. A
  compiler that folds the cast and its round-trip check would accept 2**32 as
  2**32 - 1.
flags: [onlyStrict]
---*/

assert.throws(RangeError, function () { new Array(4294967296); }, "new Array(2**32)");
assert.throws(RangeError, function () { new Array(-1.5 + 0.5); }, "new Array(-1)");
assert.throws(RangeError, function () { new Array(NaN); }, "new Array(NaN)");
assert.sameValue(new Array(4294967295).length, 4294967295, "new Array(2**32 - 1)");

var a = [];
assert.throws(RangeError, function () { a.length = 4294967296; }, "length = 2**32");
assert.throws(RangeError, function () { a.length = -1.5 + 0.5; }, "length = -1");
assert.throws(RangeError, function () { a.length = NaN; }, "length = NaN");
assert.throws(RangeError, function () { Object.defineProperty(a, "length", { value: 4294967296 }); }, "defineProperty length 2**32");
assert.sameValue(a.length, 0, "length unchanged");
a.length = 4294967295;
assert.sameValue(a.length, 4294967295, "length = 2**32 - 1");
