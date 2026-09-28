/*---
description: >
  Math.imul wraps its product to int32. fx_Math_imul (xsMath.c) multiplied two
  txInteger values, which is signed integer overflow, undefined behavior in C.
flags: [onlyStrict]
---*/

assert.sameValue(Math.imul(1073741824, 7), -1073741824, "2**30 * 7");
assert.sameValue(Math.imul(0x7FFFFFFF, 2), -2, "(2**31 - 1) * 2");
assert.sameValue(Math.imul(-2147483648, -1), -2147483648, "-(2**31) * -1");
assert.sameValue(Math.imul(0xFFFFFFFF, 5), -5, "(2**32 - 1) * 5");
assert.sameValue(Math.imul(65536, 65536), 0, "2**16 * 2**16");
assert.sameValue(Math.imul(3, 4), 12, "no overflow");
