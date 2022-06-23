/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/460
flags: [onlyStrict]
---*/

function test() {
	var set = new Set([
		[
			1,
			11
		],
		[
			2,
			22
		]
	]);
	var iterator = 'then'[Symbol.iterator]();
	iterator.next.call(set[Symbol.iterator]());
}

assert.throws(TypeError, () => test());
