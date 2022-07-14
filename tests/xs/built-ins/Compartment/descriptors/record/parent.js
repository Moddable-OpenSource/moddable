/*---
description: 
flags: [async, onlyStrict]
---*/

const foo = new StaticModuleRecord(`
	export default "foo";
`);
const bar = new StaticModuleRecord(`
	export * as foo from "foo";
`);

const c0 = new Compartment({ 
// 	modules: {
// 		foo: { record: foo },
// 		bar: { record: bar },
// 	},
	async loadHook(specifier) {
		if (specifier == "foo") return await { record:foo };
		if (specifier == "bar") return await { record:bar };
	}
});

c0.evaluate(`
	const c1 = new Compartment({
		async loadHook(specifier) {
			return await { record:specifier };
		}
	});
	const c2 = new Compartment({
		async loadHook(specifier) {
			return await { record:specifier };
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
