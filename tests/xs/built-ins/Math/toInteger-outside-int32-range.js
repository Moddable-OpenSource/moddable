/*---
description: >
  Math.ceil, Math.floor, Math.round, Math.sign, Math.trunc and Number() return
  integral results outside the int32 range unchanged, including NaN and
  Infinity. fx_Math_toInteger (xsMath.c) stores integral results in an integer
  slot after an out-of-range double to int32 cast and a round-trip check.
flags: [onlyStrict]
---*/

assert.sameValue(Math.trunc(-Infinity), -Infinity, "Math.trunc(-Infinity)");
assert.sameValue(Math.trunc(Infinity), Infinity, "Math.trunc(Infinity)");
assert.sameValue(Number.isNaN(Math.trunc(NaN)), true, "Math.trunc(NaN)");
assert.sameValue(Math.floor(2147483648.5), 0x80000000, "Math.floor above int32");
assert.sameValue(Math.ceil(-2147483648.5), -0x80000000, "Math.ceil at int32 minimum");
assert.sameValue(Math.ceil(-2147483649.5), -0x80000001, "Math.ceil below int32");
assert.sameValue(Math.round(4294967296.4), 0x100000000, "Math.round above uint32");
assert.sameValue(Math.round(Number.MAX_SAFE_INTEGER), 0x1FFFFFFFFFFFFF, "Math.round 2**53 - 1");
assert.sameValue(Math.trunc(1e10), 0x2540BE400, "Math.trunc 1e10");
assert.sameValue(Math.sign(-Infinity), -1, "Math.sign(-Infinity)");
assert.sameValue(Number("2147483648"), 0x80000000, "Number() above int32");
assert.sameValue(Number("-Infinity"), -Infinity, "Number(-Infinity)");
assert(Object.is(Math.round(-0.4), -0), "Math.round keeps -0");
assert(Object.is(Math.trunc(-0.5), -0), "Math.trunc keeps -0");
assert.sameValue(Math.trunc(2147483647.9), 0x7FFFFFFF, "Math.trunc at int32 maximum");
assert.sameValue(Math.trunc(-2147483648.9), -0x80000000, "Math.trunc at int32 minimum");
