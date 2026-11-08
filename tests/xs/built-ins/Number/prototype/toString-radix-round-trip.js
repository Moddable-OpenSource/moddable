/*---
description: Number.prototype.toString(radix) digits denote the number, so they read back to the same value
flags: [onlyStrict]
---*/

// Fraction digits must round back to the input (the old output was "0.mcc8m4rge").
assert.sameValue((0.6206327010376135).toString(36), "0.mcc8m4rge08", "(0.6206327010376135).toString(36)");

// Integer digits at or above 2^53 must be exact (the old output was "5v1j4f4ds7c000").
assert.sameValue((1e21).toString(36), "5v1j4f4ds79m9s", "(1e21).toString(36)");

// Results that were already correct must not change.
assert.sameValue((0.5).toString(2), "0.1", "(0.5).toString(2)");
assert.sameValue((255.5).toString(16), "ff.8", "(255.5).toString(16)");
assert.sameValue((-7.25).toString(8), "-7.2", "(-7.25).toString(8)");
