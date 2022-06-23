/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/285
flags: [onlyStrict]
---*/

let expected = [
	`"a"`,
	`0`,
	`"b"`,
	`1`,
	`""`,
	`{}`
];

function reviver(key, value) {
	assert(expected.length >= 2);
	assert.sameValue(JSON.stringify(key), expected.shift());
	assert.sameValue(JSON.stringify(value), expected.shift());
}

JSON.parse('{ "a": 0, "b": 1 }', reviver);
