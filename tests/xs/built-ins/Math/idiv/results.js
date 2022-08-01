/*---
description: 
flags: [onlyStrict]
---*/

assert.sameValue(Math.idiv(4, 2), 2, "(4, 2)");
assert.sameValue(Math.idiv(4, 3), 1, "(4, 3)");
assert.sameValue(Math.idiv(4, 0), NaN, "(4, 0)");
assert.sameValue(Math.idiv(0, 5), 0, "(0, 5)");
assert.sameValue(Math.idiv(4, 2.9), 2, "(4, 2.9)");
assert.sameValue(Math.idiv(4, 1.8), 4, "(4, 1.8)");
assert.sameValue(Math.idiv(10, 2.5), 5, "(10, 2.5)");
assert.sameValue(Math.idiv(3, 1.5), 3, "(3, 1.5)");

assert.sameValue(Math.idiv(-4, 2), -2, "(-4, 2)");
assert.sameValue(Math.idiv(4, -2), -2, "(4, -2)");
assert.sameValue(Math.idiv(-4, -2), 2, "(-4, -2)");

assert.sameValue(Math.idiv(5, 0xffffffff), -5, "(5, 0xffffffff)");
assert.sameValue(Math.idiv(0xfffffffe, 0xffffffff), 2, "(0xfffffffe, 0xffffffff)");

assert.sameValue(Math.idiv(0x7FFFFFFF, 0x7FFFFFFF), 1, "(0x7FFFFFFF, 0x7FFFFFFF)");
assert.sameValue(Math.idiv(0x7FFFFFFF, 2), 0x3FFFFFFF, "(0x7FFFFFFF, 2)");

assert.sameValue(Math.idiv(0x80000000, 0x80000000), 1, "(0x80000000, 0x80000000)");
assert.sameValue(Math.idiv(0x80000000, 2), -1073741824, "(0x80000000, 2)");

assert.sameValue(Math.idiv(4, Infinity), NaN, "(4, Infinity)");
assert.sameValue(Math.idiv(Infinity, 4), 0, "(Infinity, 4)");

assert.sameValue(Math.idiv(3, "bad value"), NaN, '(3, "bad value")');
assert.sameValue(Math.idiv("bad value", 3), 0, '("bad value", 3)');

assert.sameValue(Math.idiv(0xfffffffe), NaN, "(0xfffffffe)");
assert.sameValue(Math.idiv(), NaN, "()");

assert.sameValue(Math.idiv(-2147483648, -1), -2147483648, "(-2147483648, -1)");
assert.sameValue(Math.idiv(-2147483648, 1), -2147483648, "(-2147483648, 1)");
assert.sameValue(Math.idiv(2147483647, -1), -2147483647, "(2147483648, -1)");
assert.sameValue(Math.idiv(2147483647, 1), 2147483647, "(2147483648, 1)");
