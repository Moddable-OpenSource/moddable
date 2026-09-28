/*---
description: >
  TypedArray.prototype.with throws a RangeError for indices outside the array,
  including indices beyond the int32 range. fx_TypedArray_prototype_with
  (xsDataView.c) cast the relative index to txInteger before checking it,
  which is undefined behavior outside the int32 range; a wrapping conversion
  turns 2**32 into 0.
flags: [onlyStrict]
---*/

var ta = new Int8Array(4);
assert.throws(RangeError, function () { ta.with(2 ** 32, 1); }, "2**32");
assert.throws(RangeError, function () { ta.with(2 ** 32 + 1, 1); }, "2**32 + 1");
assert.throws(RangeError, function () { ta.with(-(2 ** 32), 1); }, "-(2**32)");
assert.throws(RangeError, function () { ta.with(2 ** 53, 1); }, "2**53");
assert.throws(RangeError, function () { ta.with(Infinity, 1); }, "Infinity");
assert.throws(RangeError, function () { ta.with(-Infinity, 1); }, "-Infinity");
assert.sameValue(ta.with(-1, 5)[3], 5, "-1 is the last element");

var coerced = false;
assert.throws(RangeError, function () {
	ta.with(2 ** 32, { valueOf() { coerced = true; return 1; } });
}, "value is coerced before the index is checked");
assert(coerced, "valueOf was called");
