/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/1807
flags: [onlyStrict]
---*/

function check(actual, expected) {
	if (actual !== expected)
		throw new Error(`expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
}

// With a gap, an object whose properties are all skipped must serialize as "{}"
check(JSON.stringify({ a: undefined }, null, 2), "{}");
check(JSON.stringify({ f() {} }, null, 2), "{}");
check(JSON.stringify(/x/, null, 2), "{}");
check(JSON.stringify({}, null, 2), "{}");
check(JSON.stringify({ a: undefined, b: 1 }, null, 2), '{\n  "b": 1\n}');
