/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/409
flags: [onlyStrict]
---*/

function test() {
	var x = undefined;
	for ( x in 'str' ) throw new Test262Error;
}
assert.throws(Test262Error, () => test());
