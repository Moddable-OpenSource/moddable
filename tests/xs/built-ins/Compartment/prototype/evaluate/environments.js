/*---
description: 
flags: [onlyStrict]
---*/

const c = new Compartment({
	globals: {
		foo:0,
	},
	globalLexicals: {
		bar:0,
	},
});
let result;
result = c.evaluate(`
	bar = foo++
`);
assert.sameValue(result, 0, "return");
result = c.evaluate(`
	bar = foo++
`);
assert.sameValue(result, 1, "return");
assert.sameValue(c.globalThis.foo, 2, "globals");
assert.sameValue(c.globalThis.bar, undefined, "globalLexicals");
