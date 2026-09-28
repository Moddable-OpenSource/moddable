/*---
description: >
  JSON.parse keeps integral numbers outside the int32 range.
  fxParseJSONToken (xsJSON.c) classified numbers as integers with an
  out-of-range double to int32 cast. Apple clang 21 on arm64 at -O2 and above
  folds the cast and its round-trip check, so every such number became
  2147483647.
flags: [onlyStrict]
---*/

assert.sameValue(JSON.parse("2147483647"), 0x7FFFFFFF, "2**31 - 1");
assert.sameValue(JSON.parse("2147483648"), 0x80000000, "2**31");
assert.sameValue(JSON.parse("-2147483648"), -0x80000000, "-(2**31)");
assert.sameValue(JSON.parse("-2147483649"), -0x80000001, "-(2**31) - 1");
assert.sameValue(JSON.parse("4294967296"), 0x100000000, "2**32");
assert.sameValue(JSON.parse("9007199254740991"), 0x1FFFFFFFFFFFFF, "2**53 - 1");
assert.sameValue(JSON.parse("1e10"), 0x2540BE400, "exponent form");
assert.sameValue(JSON.parse("1e400"), Infinity, "overflow to Infinity");
assert.sameValue(JSON.parse("[2147483647, 2147483648]")[1], 0x80000000, "in an array");
assert.sameValue(JSON.parse('{"a": 4294967296}').a, 0x100000000, "in an object");
assert(Object.is(JSON.parse("-0"), -0), "-0 stays -0");
