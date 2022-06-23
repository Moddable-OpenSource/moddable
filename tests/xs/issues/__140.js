/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/140
flags: [onlyStrict]
---*/

function test1() {
	function isString (thing) {
		return typeof thing === 'string';
	}

	function ensureRecipeRule (thing) {
		if (thing.referenceTo instanceof Set) {
			[...thing.referenceTo].every(type => isString(type));
		}

		// ERROR #1
		// THE EXACT SAME THING AS ABOVE REPEATED: "callback is no function"
		// if (thing.referenceTo instanceof Set) {
		//     [...thing.referenceTo].every(type => isString(type));
		// }

		// ERROR #2
		// "cannot coerce undefined to object"
		if (thing.idReferenceTo instanceof Set) {
			[...thing.idReferenceTo].every(type => isString(type));
		}
	}

	trace('Start\n');

	[
		{
			referenceTo: new Set(['Person']),
		},
		{
			idReferenceTo: new Set(['Group'])
		}
	]
	.map(rule => ensureRecipeRule(rule));
}

function test2() {
	function isString (s) {
		return typeof s === 'string';
	}

	const s1 = new Set('s1');

	[...s1].every(isString);
	[...s1].every(isString);  // the 2nd call fails}
}

function test3() {
	const a1 = [1,2,3];
	const a2 = [];

	const result = [...a1, ...a2];
	assert.sameValue("[1,2,3]", JSON.stringify(result));
}

test1();
test2();
test3();

