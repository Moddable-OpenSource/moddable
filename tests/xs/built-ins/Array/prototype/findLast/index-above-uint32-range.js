/*---
description: >
  Array.prototype.findLast gets items at indices above 2**32 - 1 on array-like
  objects. fxFindThisItem (xsArray.c) cast the index to txIndex to get the
  item, which is undefined behavior above 2**32 - 1.
includes: [compareArray.js]
flags: [onlyStrict]
---*/

var last = 2 ** 53 - 2;
var obj = { length: 2 ** 53 - 1 };
obj[last] = "right";
obj[4294967295] = "wrong";
obj[4294967294] = "wrong";

var calls = [];
var found = Array.prototype.findLast.call(obj, function (value, index) {
	calls.push(index);
	return true;
});
assert.sameValue(found, "right", "findLast gets the item at 2**53 - 2");
assert.compareArray(calls, [last], "findLast passes index 2**53 - 2");
