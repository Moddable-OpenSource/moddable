/*---
description: >
  Math.irandom returns an integer, converts its arguments with ToInt32, and is fully
  determined when the requested range holds at most one integer
flags: [onlyStrict]
---*/

assert.sameValue(typeof Math.irandom(), "number", "the result is a Number");
assert(Number.isInteger(Math.irandom()), "the result is an integer");

// A range holding a single integer leaves nothing to choose.
assert.sameValue(Math.irandom(0), 0, "irandom(0)");
assert.sameValue(Math.irandom(1), 0, "irandom(1)");
assert.sameValue(Math.irandom(0, 0), 0, "irandom(0, 0)");
assert.sameValue(Math.irandom(7, 7), 7, "irandom(7, 7)");
assert.sameValue(Math.irandom(-5, -5), -5, "irandom(-5, -5)");
assert.sameValue(Math.irandom(0, 1), 0, "irandom(0, 1)");
assert.sameValue(Math.irandom(-1, 0), -1, "irandom(-1, 0)");

// Counting down, the range is (max, min] rather than [min, max).
assert.sameValue(Math.irandom(1, 0), 1, "irandom(1, 0)");
assert.sameValue(Math.irandom(0, -1), 0, "irandom(0, -1)");
assert.sameValue(Math.irandom(-5, -6), -5, "irandom(-5, -6)");

// The ends of int32, where max - min does not fit in a signed 32-bit integer.
assert.sameValue(Math.irandom(-2147483648, -2147483647), -2147483648, "irandom at the bottom of int32");
assert.sameValue(Math.irandom(2147483646, 2147483647), 2147483646, "irandom at the top of int32");
assert.sameValue(Math.irandom(2147483647, 2147483646), 2147483647, "irandom counting down from the top of int32");

// Arguments are converted with ToInt32, which truncates toward zero.
assert.sameValue(Math.irandom(1.9), 0, "1.9 converts to 1");
assert.sameValue(Math.irandom(-5.9, -4), -5, "-5.9 truncates toward zero, not down");
assert.sameValue(Math.irandom("1"), 0, "a string converts");
assert.sameValue(Math.irandom(true), 0, "true converts to 1");
assert.sameValue(Math.irandom(undefined), 0, "undefined converts to 0");
assert.sameValue(Math.irandom(null), 0, "null converts to 0");
assert.sameValue(Math.irandom(NaN), 0, "NaN converts to 0");
assert.sameValue(Math.irandom(Infinity), 0, "Infinity converts to 0");
assert.sameValue(Math.irandom(-Infinity), 0, "-Infinity converts to 0");
assert.sameValue(Math.irandom("not a number"), 0, "an unparsable string converts to 0");

assert.throws(TypeError, function () {
	Math.irandom(Symbol());
}, "a symbol cannot be converted");

assert.throws(TypeError, function () {
	Math.irandom(0, Symbol());
}, "a symbol cannot be converted as the second argument");

// Both arguments are converted, left to right, exactly once.
const order = [];
Math.irandom(
	{ valueOf: function () { order.push("min"); return 0; } },
	{ valueOf: function () { order.push("max"); return 1; } }
);
assert.sameValue(order.join(","), "min,max", "arguments are converted left to right, once each");

// A lone argument is the maximum, not the minimum.
let converted = 0;
Math.irandom({ valueOf: function () { converted++; return 1; } });
assert.sameValue(converted, 1, "a single argument is converted once");

// Arguments past the second are ignored, not even converted.
let touched = false;
assert.sameValue(Math.irandom(7, 7, { valueOf: function () { touched = true; return 0; } }), 7,
	"arguments past the second are ignored");
assert.sameValue(touched, false, "arguments past the second are not converted");
