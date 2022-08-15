/*---
description: 
flags: [async,onlyStrict]
---*/

const foo = new ModuleSource(`
	export const x = 1;
	export const z = 3;
`);

const bar = {
	bindings:[
		{ import: "x", from: "foo" },
		{ export: "y" },
		{ export: "z", from: "foo" },
	], 
	async execute($, Import, ImportMeta) {
		assert.throws(TypeError, () => {
			$.z = 0;
		});
		assert.throws(TypeError, () => {
			delete $.y;
		});
		assert.throws(TypeError, () => {
			$.x = 0;
		});
		$.y = 2;
	} 
};

const c1 = new Compartment({ 
	resolveHook(importSpecifier, referrerSpecifier) {
		return importSpecifier;
	},
	modules: {
		foo: { source:foo },
		bar: { source:bar },
	}
});

c1.import("bar")
.then((ns) => {
	assert.sameValue(ns.x, undefined);
	assert.sameValue(ns.y, 2);
	assert.sameValue(ns.z, 3);
})
.then($DONE, $DONE);
