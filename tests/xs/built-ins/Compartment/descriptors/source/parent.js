/*---
description: 
flags: [async, onlyStrict]
---*/

const foo = new ModuleSource(`
	export default "foo";
`);
const bar = new ModuleSource(`
	export * as foo from "foo";
`);

const c0 = new Compartment({ 
// 	modules: {
// 		foo: { source: foo },
// 		bar: { source: bar },
// 	},
	resolveHook(importSpecifier, referrerSpecifier) {
		return importSpecifier;
	},
	async loadHook(specifier) {
		if (specifier == "foo") return await { source:foo };
		if (specifier == "bar") return await { source:bar };
	}
});

c0.evaluate(`
	const c1 = new Compartment({
		async loadHook(specifier) {
			return await { source:specifier };
		}
	});
	const c2 = new Compartment({
		async loadHook(specifier) {
			return await { source:specifier };
		}
	});
	Promise.all([
		c1.import("bar"),
		c2.import("bar"),
	])
`)
.then(([ bar1, bar2 ]) => {
	assert(bar1.foo != bar2.foo, "foo is separate");
	assert(bar1 != bar2, "bar is separate");
	
})
.then($DONE, $DONE);
