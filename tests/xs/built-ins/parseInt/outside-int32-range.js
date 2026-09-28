/*---
description: >
  parseInt returns results outside the int32 range unchanged. fx_parseInt
  (xsNumber.c) stores integral results in an integer slot after an out-of-range
  double to int32 cast and a round-trip check.
flags: [onlyStrict]
---*/

assert.sameValue(parseInt("2147483647"), 0x7FFFFFFF, "2**31 - 1");
assert.sameValue(parseInt("2147483648"), 0x80000000, "2**31");
assert.sameValue(parseInt("-2147483648"), -0x80000000, "-(2**31)");
assert.sameValue(parseInt("-2147483649"), -0x80000001, "-(2**31) - 1");
assert.sameValue(parseInt("4294967296"), 0x100000000, "2**32");
assert.sameValue(parseInt("ffffffff", 16), 0xFFFFFFFF, "radix 16");
assert.sameValue(parseInt("0x100000000"), 0x100000000, "hex prefix");
assert.sameValue(parseInt("1".repeat(400)), Infinity, "overflow to Infinity");
assert(Object.is(parseInt("-0"), -0), "-0 stays -0");
