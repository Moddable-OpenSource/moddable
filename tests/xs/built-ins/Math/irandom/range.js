/*---
description: Math.irandom never returns a value outside the half-open range it was given
info: |
  Counting up the result is in [min, max); counting down (max < min) it is in (max, min].
  The range may span the whole of int32, so max - min does not always fit in a signed
  32-bit integer. A platform random number generator that returns a value outside
  [0, C_RAND_MAX] pushes the result past one of these bounds, so this is also the check
  that the generator itself is in range.
flags: [onlyStrict]
---*/

const INT32_MIN = -2147483648;
const INT32_MAX = 2147483647;
const SAMPLES = 1000;

function sample(label, draw, lo, hi) {
	for (let i = 0; i < SAMPLES; i++) {
		const value = draw();
		if (!Number.isInteger(value) || (value < lo) || (value > hi))
			assert(false, label + " returned " + value + ", outside [" + lo + ", " + hi + "]");
	}
}

// Counting up: [min, max)
sample("irandom(0, 10)", function () { return Math.irandom(0, 10); }, 0, 9);
sample("irandom(5, 10)", function () { return Math.irandom(5, 10); }, 5, 9);
sample("irandom(-10, -5)", function () { return Math.irandom(-10, -5); }, -10, -6);
sample("irandom(-3, 3)", function () { return Math.irandom(-3, 3); }, -3, 2);

// Counting down: (max, min]
sample("irandom(10, 5)", function () { return Math.irandom(10, 5); }, 6, 10);
sample("irandom(-5, -10)", function () { return Math.irandom(-5, -10); }, -9, -5);
sample("irandom(3, -3)", function () { return Math.irandom(3, -3); }, -2, 3);

// One argument is the maximum; a negative one therefore counts down from zero.
sample("irandom(10)", function () { return Math.irandom(10); }, 0, 9);
sample("irandom(-10)", function () { return Math.irandom(-10); }, -9, 0);

// No arguments: [0, 2147483647)
sample("irandom()", function () { return Math.irandom(); }, 0, INT32_MAX - 1);

// Ranges that span more of int32 than a signed 32-bit integer can hold.
sample("irandom(0, INT32_MAX)", function () { return Math.irandom(0, INT32_MAX); }, 0, INT32_MAX - 1);
sample("irandom(INT32_MIN, 0)", function () { return Math.irandom(INT32_MIN, 0); }, INT32_MIN, -1);
sample("irandom(INT32_MIN, INT32_MAX)", function () { return Math.irandom(INT32_MIN, INT32_MAX); }, INT32_MIN, INT32_MAX - 1);
sample("irandom(INT32_MAX, INT32_MIN)", function () { return Math.irandom(INT32_MAX, INT32_MIN); }, INT32_MIN + 1, INT32_MAX);

// The narrowest ranges at the extremes, where an off-by-one has nowhere to hide.
sample("irandom(INT32_MIN, INT32_MIN + 1)", function () { return Math.irandom(INT32_MIN, INT32_MIN + 1); }, INT32_MIN, INT32_MIN);
sample("irandom(INT32_MAX - 1, INT32_MAX)", function () { return Math.irandom(INT32_MAX - 1, INT32_MAX); }, INT32_MAX - 1, INT32_MAX - 1);
sample("irandom(INT32_MIN + 1, INT32_MIN)", function () { return Math.irandom(INT32_MIN + 1, INT32_MIN); }, INT32_MIN + 1, INT32_MIN + 1);
sample("irandom(INT32_MAX, INT32_MAX - 1)", function () { return Math.irandom(INT32_MAX, INT32_MAX - 1); }, INT32_MAX, INT32_MAX);
