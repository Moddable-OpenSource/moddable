/*---
description: >
  String property keys outside the array index range are not array indices.
  fxStringToIndex (xsCommon.c) casts the numeric value of the key to txIndex
  before checking it, which is undefined behavior for negative or too large
  keys.
includes: [compareArray.js]
flags: [onlyStrict]
---*/

var a = [];
a["-1"] = "minus one";
assert.sameValue(a.length, 0, "'-1' does not set length");
assert.sameValue(a[0], undefined, "'-1' does not set a[0]");
a["4294967295"] = "not an index";
assert.sameValue(a.length, 0, "'4294967295' is not an array index");
a["4294967296"] = "not an index";
assert.sameValue(a.length, 0, "'4294967296' is not an array index");
a["4294967294"] = "last index";
assert.sameValue(a.length, 4294967295, "'4294967294' is the last array index");

var o = { b: 1, "4294967296": 2, "1": 3, "-1": 4, "4294967294": 5 };
assert.compareArray(Reflect.ownKeys(o), ["1", "4294967294", "b", "4294967296", "-1"], "only array indices are ordered first");
