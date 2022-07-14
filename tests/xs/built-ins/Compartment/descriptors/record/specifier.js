/*---
description: 
flags: [async,onlyStrict]
---*/

const foo = new StaticModuleRecord(`
	export default "foo";
`);
const bar = new StaticModuleRecord(`
	import foo from "./foo";
	export default foo + "bar";
`);

const c1 = new Compartment({ 
	modules: {
		"foo": { record: foo },
		"bar": { record: bar, specifier:"BAR" },
	},
	resolveHook(specifier, referrerSpecifier) {
		assert.sameValue(referrerSpecifier, "BAR");
		return "foo";
	}
});

Promise.all([
	c1.import("bar"),
])
.then(([ bar ]) => {
	assert.sameValue(bar.default, "foobar");
})
.then($DONE, $DONE);
