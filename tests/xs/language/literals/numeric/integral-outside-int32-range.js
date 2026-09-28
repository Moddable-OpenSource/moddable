/*---
description: >
  Integral numeric literals outside the int32 range keep their value.
  fxGetNextNumber (xsLexical.c) classified literals as integers with an
  out-of-range double to int32 cast. Apple clang 21 on arm64 at -O2 and above
  folds the cast and its round-trip check, so every such literal became
  2147483647. Hexadecimal literals take a different path and are used as the
  reference values.
flags: [onlyStrict]
---*/

assert.sameValue(2147483647, 0x7FFFFFFF, "2**31 - 1");
assert.sameValue(2147483648, 0x80000000, "2**31");
assert.sameValue(-2147483648, -0x80000000, "-(2**31)");
assert.sameValue(-2147483649, -0x80000001, "-(2**31) - 1");
assert.sameValue(4294967295, 0xFFFFFFFF, "2**32 - 1");
assert.sameValue(4294967296, 0x100000000, "2**32");
assert.sameValue(9007199254740991, 0x1FFFFFFFFFFFFF, "2**53 - 1");
assert.sameValue(21474836480, 0x500000000, "10 * 2**31");
assert.sameValue(1e10, 0x2540BE400, "exponent form");
assert.sameValue(4_294_967_296, 0x100000000, "numeric separators");
assert.sameValue(2147483648.0, 0x80000000, "trailing .0");
assert.sameValue(1e308 * 10, Infinity, "1e308 is not clamped");
assert.sameValue(typeof 2147483648, "number");
assert.sameValue(eval("4294967296"), 0x100000000, "eval");
assert.sameValue(new Function("return 4294967296")(), 0x100000000, "Function");
assert.sameValue(2147483647 + 1, 0x80000000, "int32 maximum still an integer");
