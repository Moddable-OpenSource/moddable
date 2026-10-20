/*---
description: toFixed, toExponential, and toPrecision pick the larger value when two candidates are equally close
flags: [onlyStrict]
---*/

const fixed = [
	[0.5, 0, "1"], [1.5, 0, "2"], [2.5, 0, "3"], [-0.5, 0, "-1"], [-2.5, 0, "-3"],
	[0.125, 2, "0.13"], [0.375, 2, "0.38"], [1.25, 1, "1.3"], [1.0625, 3, "1.063"],
	[23575005085478008, 2, "23575005085478008.00"],
];
for (const [value, digits, expected] of fixed)
	assert.sameValue(value.toFixed(digits), expected, `(${value}).toFixed(${digits})`);

const exponential = [
	[0.5, 0, "5e-1"], [2.5, 0, "3e+0"], [25, 0, "3e+1"], [12.5, 1, "1.3e+1"], [1.25, 1, "1.3e+0"],
	[1.125, 2, "1.13e+0"], [-12.5, 1, "-1.3e+1"], [1024, 2, "1.02e+3"],
];
for (const [value, digits, expected] of exponential)
	assert.sameValue(value.toExponential(digits), expected, `(${value}).toExponential(${digits})`);

const precision = [
	[0.5, 1, "0.5"], [2.5, 1, "3"], [12.5, 2, "13"], [1.25, 2, "1.3"], [1.125, 3, "1.13"],
	[-12.5, 2, "-13"], [1024, 3, "1.02e+3"],
];
for (const [value, digits, expected] of precision)
	assert.sameValue(value.toPrecision(digits), expected, `(${value}).toPrecision(${digits})`);
