/*---
description: >
  Array.prototype.findLastIndex returns indices above 2**32 - 1 on array-like
  objects. fx_Array_prototype_findLastIndex (xsArray.c) cast the result to
  txUnsigned, which is undefined behavior above 2**32 - 1.
flags: [onlyStrict]
---*/

var last = 2 ** 53 - 2;
var obj = { length: 2 ** 53 - 1 };
var foundIndex = Array.prototype.findLastIndex.call(obj, function (value, index) {
	return index === last;
});
assert.sameValue(foundIndex, last, "findLastIndex returns 2**53 - 2");
