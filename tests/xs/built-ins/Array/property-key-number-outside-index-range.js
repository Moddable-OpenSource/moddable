/*---
description: >
  Number property keys outside the array index range are not array indices.
  fxNumberToIndex (xsCommon.c) casts the key to txIndex before checking it,
  which is undefined behavior for negative, too large or NaN keys. A compiler
  that folds the cast and its round-trip check would turn -1 into index 0.
flags: [onlyStrict]
---*/

var minusOne = -1.5 + 0.5;
var a = [];
a[minusOne] = "minus one";
assert.sameValue(a.length, 0, "a[-1] does not set length");
assert.sameValue(a[0], undefined, "a[-1] does not set a[0]");
assert.sameValue(a["-1"], "minus one", "a[-1] is the property '-1'");

a[NaN] = "nan";
assert.sameValue(a.length, 0, "a[NaN] does not set length");
assert.sameValue(a.NaN, "nan", "a[NaN] is the property 'NaN'");

a[4294967295] = "not an index";
assert.sameValue(a.length, 0, "2**32 - 1 is not an array index");
a[4294967296] = "not an index";
assert.sameValue(a.length, 0, "2**32 is not an array index");
a[1e21] = "not an index";
assert.sameValue(a.length, 0, "1e21 is not an array index");
assert.sameValue(a["1e+21"], "not an index", "1e21 is the property '1e+21'");

a[4294967294] = "last index";
assert.sameValue(a.length, 4294967295, "2**32 - 2 is the last array index");
