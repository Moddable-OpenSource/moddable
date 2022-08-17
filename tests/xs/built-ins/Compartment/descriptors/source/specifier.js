/*---
description: 
flags: [async,onlyStrict]
---*/

const foo = new ModuleSource(`
	export default "foo";
`);
const bar = new ModuleSource(`
	import foo from "./foo";
	export default foo + "bar";
`);

const c1 = new Compartment({ 
	resolveHook(importSpecifier, referrerSpecifier) {
		assert.sameValue(referrerSpecifier, "BAR");
		return "foo";
	},
	modules: {
		"foo": { source: foo },
		"bar": { source: bar, specifier:"BAR" },
	}
});

Promise.all([
	c1.import("bar"),
])
.then(([ bar ]) => {
	assert.sameValue(bar.default, "foobar");
})
.then($DONE, $DONE);
