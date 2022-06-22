/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/453
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
	var iterator = [0,,,'3'][Symbol.iterator]();
	iterator.next.call(set[Symbol.iterator]());
}

assert.throws(TypeError, () => test());
